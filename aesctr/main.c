#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/stat.h>

#include "sha2.h"
#include "aes.h"
#include "color_print.h"

#define BUFFLEN 1024

int g_debug = 0;

char g_infile[BUFFLEN];
int g_infile_specified = 0;
int g_infile_fd;

char g_outfile[BUFFLEN];
int g_outfile_specified = 0;
int g_outfile_fd;
int g_outfile_overwrite = 0;

char g_keyfile[BUFFLEN];
int g_keyfile_specified = 0;
uint8_t g_key[32];
uint8_t g_iv[16];
char g_passphrase[BUFFLEN];
int g_passphrase_specified = 0;
int g_pin = 1000;
// passphrase handling stuff
char g_forward_hash[BUFFLEN];
char g_new[64];


int g_urandom_fd;

typedef enum {
    MODE_NONE,
    MODE_PROCESS,
    MODE_GENERATE
} operational_mode;

operational_mode g_mode = MODE_NONE;

struct option g_options[] = {
    { "help", no_argument, NULL, '?' },
    { "debug", no_argument, NULL, 1001 },
    { "in", required_argument, NULL, 'i' },
    { "out", required_argument, NULL, 'o' },
    { "key", required_argument, NULL, 'k' },
    { "process", no_argument, NULL, 'p' },
    { "generate", no_argument, NULL, 'g' },
    { "overwrite", no_argument, NULL, 'w' },
    { "passphrase", required_argument, NULL, 1002 },
    { "pin", required_argument, NULL, 1003 },
    { NULL, 0, NULL, 0 }
};

void print_hex(uint8_t *a_buffer, size_t a_len)
{
    int i;
    for (i = 0; i < a_len; ++i) {
        if (i % 32 == 0)
            printf("\n");
        printf("%02X ", a_buffer[i]);
    }
    printf("\n");
}

void load_key()
{
    if (g_keyfile_specified == 0) {
        color_err_printf(0, "aesctr: this operation requires that you specify a key file.");
        exit(EXIT_FAILURE);
    }
    int key_fd;
    int res;
    key_fd = open(g_keyfile, O_RDONLY);
    if (key_fd < 0) {
        color_err_printf(1, "aesctr: unable to open key file");
        exit(EXIT_FAILURE);
    }
    res = read(key_fd, g_key, 32);
    if (res < 0) {
        color_err_printf(1, "aesctr: unable to read key file");
        exit(EXIT_FAILURE);
    }
    res = read(key_fd, g_iv, 16);
    if (res < 0) {
        color_err_printf(1, "aesctr: unable to read key file");
        exit(EXIT_FAILURE);
    }
    close(key_fd);
    if (g_debug > 0) {
        color_debug("load_key: loaded key");
        print_hex(g_key, 32);
        color_debug("load_key: loaded iv");
        print_hex(g_iv, 16);
    }
}

void prepare_outfile()
{
    int res;

    // find out if outfile exists
    struct stat l_outfile_stat;
    res = stat(g_outfile, &l_outfile_stat);
    if (res == 0) {
        // successfully stat-ted the file. do we want to overwrite it?
        if (g_outfile_overwrite == 0) {
            color_err_printf(0, "aesctr: output file already exists (use -w or --overwrite to write to it anyway)");
            exit(EXIT_FAILURE);
        } else {
            color_printf("*aaesctr:*d overwriting existing output file *b%s*b\n", g_outfile);
        }
    } else if ((res < 0) && (errno == ENOENT)) {
        // this is what we want
    } else {
        // some other error from stat!
        color_err_printf(1, "aesctr: unable to stat output file to check its existence");
        exit(EXIT_FAILURE);
    }

    // open the output file
    color_debug("prepare_outfile: opening and truncating output file\n");
    g_outfile_fd = open(g_outfile, O_RDWR | O_TRUNC | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if (g_outfile_fd < 0) {
        color_err_printf(1, "aesctr: error opening output file");
        exit(EXIT_FAILURE);
    }
}

void prepare_infile()
{
    int res;

    // find out infile length
    struct stat l_infile_stat;
    res = stat(g_infile, &l_infile_stat);
    if (res < 0) {
        color_err_printf(1, "aesctr: error calling stat on input file");
        exit(EXIT_FAILURE);
    }
    // open infile
    g_infile_fd = open(g_infile, O_RDONLY);
    if (g_infile_fd < 0) {
        color_err_printf(1, "aesctr: problems opening input file");
        exit(EXIT_FAILURE);
    }
}

void get_random(uint8_t *a_buffer, size_t a_len)
{
    int res;
    res = read(g_urandom_fd, a_buffer, a_len);
    if (res != a_len) {
        color_err_printf(1, "aesctr: problems reading /dev/urandom");
        exit(EXIT_FAILURE);
    }
}

void do_process()
{
    uint8_t l_buff[4096];
    int res;

    struct AES_ctx l_ctx;
    AES_init_ctx_iv(&l_ctx, g_key, g_iv);

    color_printf("*aaesctr:*d processing input file into output file...\n");
    do {
        res = read(g_infile_fd, l_buff, 4096);
        if (res == 0) {
            // EOF
            continue;
        }
        if (res < 0) {
            color_err_printf(1, "aesctr: unable to read from input file");
            exit(EXIT_FAILURE);
        }
        AES_CTR_xcrypt_buffer(&l_ctx, l_buff, res);
        res = write(g_outfile_fd, l_buff, res);
        if (res < 0) {
            color_err_printf(1, "aesctr: unable to write to output file");
            exit(EXIT_FAILURE);
        }
    } while (res != 0);

    close(g_infile_fd);
    close(g_outfile_fd);
}

void do_generate()
{
    // write 48 random bytes OR deterministic hashed data to g_keyfile
    int res;
    int key_fd;
    struct stat l_keyfile_stat;

    res = stat(g_keyfile, &l_keyfile_stat);
    if (res == 0) {
        if (g_outfile_overwrite == 0) {
            color_err_printf(0, "aesctr: key file already exists (use -w or --overwrite to write to it anyway)");
            exit(EXIT_FAILURE);
        } else {
            color_printf("*aaesctr:*d overwriting existing key file *b%s*d\n", g_outfile);
        }
    } else if ((res < 0) && (errno == ENOENT)) {
	// we want the file to not exist, this is not an error
    } else {
        color_err_printf(1, "aesctr: unable to stat key file to check its existence");
        exit(EXIT_FAILURE);
    }

    key_fd = open(g_keyfile, O_RDWR | O_TRUNC | O_CREAT, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
    if (key_fd < 0) {
        color_err_printf(1, "aesctr: error opening key file for writing");
        exit(EXIT_FAILURE);
    }
    if (g_passphrase_specified) {
	strcpy(g_forward_hash, g_passphrase);
	for (unsigned int i = 1; i <= g_pin; ++i) {
	    sha512_ctx ctx;
	    sha512_init(&ctx);
	    // first time through the loop we use the length of our passphrase string, every time after it's the length of a sha512 hash
	    sha512_update(&ctx, g_forward_hash, ((i == 1) ? strlen(g_forward_hash) : 64));
	    sha512_final(&ctx, g_new);
	    memcpy(g_forward_hash, g_new, 64);
	}
	memcpy(g_key, g_forward_hash, 32);
    } else {
    	get_random(g_key, 32);
    }
    res = write(key_fd, g_key, 32);
    if (res < 0) {
        color_err_printf(1, "aesctr: unable to write to key file");
        exit(EXIT_FAILURE);
    }
    if (g_passphrase_specified) {
	memcpy(g_iv, g_forward_hash + 32, 16);
    } else {
        get_random(g_iv, 16);
    }
    res = write(key_fd, g_iv, 16);
    if (res < 0) {
        color_err_printf(1, "aesctr: unable to write to key file");
        exit(EXIT_FAILURE);
    }
    if (g_debug > 0) {
        color_debug("do_generate: generated key");
        print_hex(g_key, 32);
        color_debug("do_generate: generated iv");
        print_hex(g_iv, 16);
    }
    close(key_fd);
}

int main(int argc, char **argv)
{
    color_init(0, g_debug);
    color_set_theme(THEME_GREEN);

    unsigned int i;
    int res; // result variable for UNIX reads
    int opt;
    while ((opt = getopt_long(argc, argv, "i:o:k:pg?w", g_options, NULL)) != -1) {
        switch (opt) {
            case 1001:
            {
                g_debug = 1;
		color_set_debug(g_debug);
            }
            break;
	    case 1002:
	    {
		strcpy(g_passphrase, optarg);
		g_passphrase_specified = 1;
	    }
	    break;
	    case 1003:
	    {
		g_pin = atoi(optarg);
	    }
	    break;
            case 'i':
            {
                strcpy(g_infile, optarg);
                g_infile_specified = 1;
            }
            break;
            case 'o':
            {
                strcpy(g_outfile, optarg);
                g_outfile_specified = 1;
            }
            break;
            case 'k':
            {
                strcpy(g_keyfile, optarg);
                g_keyfile_specified = 1;
            }
            break;
            case 'w':
            {
                g_outfile_overwrite = 1;
            }
            break;
            case 'p':
            {
                if (g_mode != MODE_NONE) {
                    color_err_printf(0, "aesctr: please select only one operational mode.");
                    exit(EXIT_FAILURE);
                }
                g_mode = MODE_PROCESS;
            }
            break;
            case 'g':
            {
                if (g_mode != MODE_NONE) {
                    color_err_printf(0, "aesctr: please select only one operational mode.");
                    exit(EXIT_FAILURE);
                }
                g_mode = MODE_GENERATE;
            }
            break;
            case '?':
            {
                color_printf("*hAES256 CTR Mode file encryptor*d\n");
		color_printf("*aBy Stephen Sviatko - version: *h1.00*d\n");
		color_printf("*adate: *h01/Oct/2026*d\n");
                color_printf("*ausage: aesctr <options>*d\n");
                color_printf("*a  -i (--in) <name>*d specify input file\n");
                color_printf("*a  -o (--out) <name>*d specify output file\n");
                color_printf("*a  -k (--key) <name>*d specify full name of key file to use\n");
                color_printf("*a  -w (--overwrite)*d force overwrite of existing output file or key file\n");
                color_printf("*a     (--debug)*d use debug mode\n");
                color_printf("*a  -? (--help)*d this screen\n");
                color_printf("*aoperational modes (select only one)*d\n");
                color_printf("*a  -p (--process)*d encrypt/decrypt in->out with specified key\n");
                color_printf("*a  -g (--generate)*d create AES256 keys\n");
                color_printf("       -g by itself writes random key to file specified by -k or --key\n");
		color_printf("       use passphrase with optional PIN to generate a specific key.\n");
                color_printf("       *bWARNING*d - use key only once or security will be compromised\n");
		color_printf("*a     (--passphrase) <phrase>*d use passphrase to generate key\n");
		color_printf("*a     (--pin) <PIN>*d specify an optional PIN to use with passphrase (1000-9999).\n");
		color_printf("       PIN will default to 1000 if unspecified.\n");
                color_printf("*aexamples*d\n");
                color_printf("*a  aesctr -gk <keyfile>*d  Generate new random key and save to <keyfile>\n");
		color_printf("*a  aesctr -gk <keyfile> --passphrase \"my phrase\" --pin 1234*d\n");
		color_printf("       Generate deterministic AES key hashed from passphrase/PIN.\n");
                color_printf(" *a aesctr -p -i <infile> -o <outfile> -k <keyfile>*d  Process in->out\n");
                exit(EXIT_SUCCESS);
            }
            break;
        }
    }

    setbuf(stdout, NULL); // disable buffering so we can print our progress

    // police PIN to between 1000 and 9999
    if ((g_pin < 1000) || (g_pin > 9999)) {
	    color_err_printf(0, "aesctr: PIN must be between 1000-9999.");
	    exit(EXIT_FAILURE);
    }

    if (g_debug > 0)
        color_printf("*aaesctr:*d debug mode *benabled.*d\n");

    if (g_infile_specified > 0) {
        color_printf("*aaesctr:*d input file : *b%s*d\n", g_infile);
    }
    if (g_outfile_specified > 0) {
        color_printf("*aaesctr:*d output file: *b%s*d\n", g_outfile);
    }
    if (g_keyfile_specified > 0) {
        color_printf("*aaesctr:*d key file   : *b%s*d\n", g_keyfile);
    }

    // prepare urandom
    g_urandom_fd = open("/dev/urandom", O_RDONLY);
    if (g_urandom_fd < 0) {
        color_err_printf(1, "aesctr: problems opening /dev/urandom");
        exit(EXIT_FAILURE);
    }

    switch (g_mode) {
        case MODE_NONE:
        {
            color_err_printf(0, "aesctr: you must select one operational mode.");
            color_err_printf(0, "aesctr: use -? or --help for usage info.");
            exit(EXIT_FAILURE);
        }
        break;
        case MODE_PROCESS:
        {
            color_printf("*aaesctr:*d selected *bprocess*d mode.\n");
            load_key();
            if (g_infile_specified == 0) {
                color_err_printf(0, "aesctr: this function requires that you specify an input file.");
                exit(EXIT_FAILURE);
            }
            prepare_infile();
            if (g_outfile_specified == 0) {
                color_err_printf(0, "aesctr: this function requires that you specify an output file.");
                exit(EXIT_FAILURE);
            }
            prepare_outfile();
            do_process();
        }
        break;
        case MODE_GENERATE:
        {
            color_printf("*aaesctr:*d selected *bgenerate*d mode.\n");
            if (g_keyfile_specified == 0) {
                color_err_printf(0, "aesctr: this function requires that you specify a keyfile to write.");
                exit(EXIT_FAILURE);
            }
	    if (g_passphrase_specified == 1) {
		    color_printf("*aaesctr:*d using passphrase *b%s*d.\n", g_passphrase);
		    color_printf("*aaesctr:*d using PIN *b%d*d.\n", g_pin);
	    }
            do_generate();
            color_printf("*aaesctr:*d *bWARNING*d - do not use this key more than once or security will be compromised.\n");
        }
        break;
        default:
        {
            printf("I don't know what to do!\n");
        }
        break;
    }

    return 0;
}
