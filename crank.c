#include <secp256k1.h>

#include <stdio.h>

#include <string.h>

#include <stdlib.h>

#include <fcntl.h>

#include <unistd.h>

#include <getopt.h>

#include "keccak256.c"

#include "cputils.c"

int gen_address(void * privkey, void * hash_of_public) {
  static char pubkey_serialized[65];
  static SHA3_CTX ctx2;
  static secp256k1_pubkey pubkey;
  static secp256k1_context * ctx;
  static size_t pubkey_len = 65;

  ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

  if (ctx == NULL) {
    return -1;
  }

  if (!secp256k1_ec_seckey_verify(ctx, privkey)) {
    secp256k1_context_destroy(ctx);
    return -2;
  }

  if (!secp256k1_ec_pubkey_create(ctx, & pubkey, privkey)) {
    secp256k1_context_destroy(ctx);
    return -3;
  }

  secp256k1_ec_pubkey_serialize(ctx, pubkey_serialized, & pubkey_len, & pubkey, SECP256K1_EC_UNCOMPRESSED);

  keccak_init( & ctx2);

  keccak_update( & ctx2, pubkey_serialized + 1, 64);

  keccak_final( & ctx2, hash_of_public);

  secp256k1_context_destroy(ctx);

  return 0;
}

int cmp20(char * a, char * b);

inline int cmp20(char * a, char * b) {
  return ((long * )(a))[0] == ((long * )(b))[0] && ((long * )(a))[1] == ((long * )(b))[1] && ((int * )(a))[4] == ((int * )(b))[4];
}

int check_all_addrs(void * seed, void * target) {
  static char hash_of_public[32];
  static int n;
  static int i;
  static int j;

  n = 0;
  i = 0;

  for (; i < 32; i++) {
    j = 0;
    for (; j < 256; j++) {

      ((char * )(seed))[i] += (char) j;

      n = gen_address(seed, hash_of_public);

      ((char * )(seed))[i] -= (char) j;

      if (n == 0 && cmp20(hash_of_public + 12, target) != 0) {
        return 0;
      }
    }
  }
  return -4;
}

void print_hex(const char * data, size_t len) {
  static size_t i;
  i = 0;
  for (; i < len; i++) {
    printf("%02x", (unsigned char) data[i]);
  }
  printf("\n");
}

int hash_in_place(char * msg) {

  static SHA3_CTX ctx3;

  keccak_init( & ctx3);

  keccak_update( & ctx3, msg, 32);

  keccak_final( & ctx3, msg);

}

int hex_to_bytes_fixed(const char * hex_str, unsigned char * bytes_out, int bytes_out_len) {
  // Verify that hex_str is exactly 64 characters long
  if (strlen(hex_str) != (bytes_out_len * 2)) {
    return -1; // Invalid length
  }

  for (int i = 0; i < bytes_out_len; i++) {
    char c1 = hex_str[2 * i];
    char c2 = hex_str[2 * i + 1];
    int high, low;

    // Convert first hex character to its numerical value
    if (c1 >= '0' && c1 <= '9') {
      high = c1 - '0';
    } else if (c1 >= 'a' && c1 <= 'f') {
      high = c1 - 'a' + 10;
    } else if (c1 >= 'A' && c1 <= 'F') {
      high = c1 - 'A' + 10;
    } else {
      return -1; // Invalid character
    }

    // Convert second hex character to its numerical value
    if (c2 >= '0' && c2 <= '9') {
      low = c2 - '0';
    } else if (c2 >= 'a' && c2 <= 'f') {
      low = c2 - 'a' + 10;
    } else if (c2 >= 'A' && c2 <= 'F') {
      low = c2 - 'A' + 10;
    } else {
      return -1; // Invalid character
    }

    // Combine the two nibbles into a single byte
    bytes_out[i] = (high << 4) | low;
  }

  return 0;
}

int main(int argc, char * argv[]) {

  int aff = -1;

  static int round = 1;

  round = 1;

  char seed[32];

  char target[20];

  int is_seed_set = 0;

  int is_target_set = 0;

  int is_affinity_set = 0;

  int opt;

  static struct option long_options[] = {
    {
      "target",
      required_argument,
      0,
      't'
    },
    {
      "affinity",
      required_argument,
      0,
      'a'
    },
    {
      "round",
      required_argument,
      0,
      'r'
    },
    {
      "seed",
      required_argument,
      0,
      's'
    },
    {
      0,
      0,
      0,
      0
    }
  };

  int long_index = 0;
  while ((opt = getopt_long(argc, argv, "t:a:r:s:", long_options, & long_index)) != -1) {
    switch (opt) {
    case 't':
      if (0 != hex_to_bytes_fixed(optarg, target, 20)) {
        errno = EINVAL;
        perror("invalid target");
        return 1;
      }
      is_target_set = 1;
      break;
    case 'a':
      aff = atoi(optarg);
      is_affinity_set = 1;
      break;
    case 'r':
      round = atoi(optarg);
      break;
    case 's':
      if (0 != hex_to_bytes_fixed(optarg, seed, 32)) {
        errno = EINVAL;
        perror("invalid seed");
        return 1;
      }
      is_seed_set = 1;
      break;
    default:
      errno = EINVAL;
      printf("usage beta: %s --target HEX [--seed HEX] [--affinity NUM] [--round NUM]\n", argv[0]);
      perror("invalid usage");
      return 1;
    }
  }

  if (optind < argc) {
    errno = EINVAL;
    perror("non-option arguments");
    return 1;
  }

  if (is_target_set == 0) {
    errno = EINVAL;
    perror("target option is not set");
    return 1;
  } else {
    printf("target: ");
    print_hex(target, 20);
  }

  if (is_seed_set == 0) {

    int fd = open("/dev/random", O_RDONLY);

    if (0 >= fd) {
      errno = fd;
      perror( "failed to read /dev/random");
      return 1;
    }

    int n = read(fd, seed, 32);

    if (n != 32) {
      errno = EINVAL;
      perror("invalid random seed length");
      return 1;
    }


    close(fd);
    printf("random seed: ");
  } else {
    printf("seed: ");
  }

  print_hex(seed, 32);

  if (is_affinity_set != 0) {
    if (0 != set_current_thread_affinity(aff)) {
      perror("failed to set cpu affinity");
      return 1;
    }
    if (set_realtime_scheduler(SCHED_FIFO, 99) != 0) {
      perror("failed to set realtime scheduler");
      return 1;
    }
    printf("affinity: %d\n", aff);
  }

  if (round <= 0) {
    errno = EINVAL;
    perror("invalid round");
    return 1;
  } else {
    printf("round: %d\n", round);
  }

  static int result = 0;

  for (int y = 0; y < round; y++) {

    result = check_all_addrs(seed, target);

    if (result == 0) {
      printf("found: ");
      print_hex(seed, 32);
      return 0;
    }
    if (result != -4) {
      errno = EINVAL;
      return 1;
    }

    hash_in_place(seed);

  }

  if (is_affinity_set != 0) {

    reset_scheduler();
    reset_cpu_affinity();

  }

  fprintf(stderr, "not found\n");

  return 2;

}