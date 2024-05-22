// engler, cs140e: driver for "bootloader" for an r/pi connected via 
// a tty-USB device.
//
// most of it is argument parsing.
//
// Unless you know what you are doing:
//              DO NOT MODIFY THIS CODE!
//              DO NOT MODIFY THIS CODE!
//              DO NOT MODIFY THIS CODE!
//              DO NOT MODIFY THIS CODE!
//              DO NOT MODIFY THIS CODE!
//
// You shouldn't have to modify any code in this file.  Though, if you find
// a bug or improve it, let us know!
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "libunix.h"
#include "put-code.h"

static char *progname = 0;

static void usage(const char *msg, ...) {
    va_list args;
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);

    output("\nusage: %s  [--trace-all] [--trace-control] ([device] | [--last] | [--first] [--device <device>]) <pi-program> \n", progname);
    output("    pi-program = has a '.bin' suffix\n");
    output("    specify a device using any method:\n");
    output("        <device>: has a '/dev' prefix\n");
    output("       --last: gets the last serial device mounted\n");
    output("        --first: gets the first serial device mounted\n");
    output("        --device <device>: manually specify <device>\n");
    output("    --baud <baud_rate>: manually specify baud_rate\n");
    output("    --trace-all: trace all put/get between rpi and unix side\n");
    output("    --trace-control: trace only control [no data] messages\n");
    exit(1);
}

enum {
  GDB_PORT = 3333,
  LISTEN_BACKLOG = 50
};

uint8_t hex_to_int(uint8_t hex) {
  if ('0' <= hex && hex <= '9')
    return hex - '0';
  return hex - 'a' + 10;
}

char int_to_hex(int x) {
  if (0 <= x && x <= 9)
    return x + '0';
  return x - 10 + 'a';
}

uint8_t gdb_get8(FILE *in) {
  return hex_to_int(fgetc(in)) * 16 + hex_to_int(fgetc(in));
}

void gdb_put8(FILE *out, uint8_t x) {
  fputc(int_to_hex(x >> 4), out);
  fputc(int_to_hex(x & 0xf), out);
}

uint8_t gdb_read8(const char *str) {
  return hex_to_int(str[0]) * 16 + hex_to_int(str[1]);
}

uint32_t gdb_read32(const char *str) {
  return (uint32_t)gdb_read8(str)     << 24 |
         (uint32_t)gdb_read8(str + 2) << 16 |
         (uint32_t)gdb_read8(str + 4) <<  8 |
         (uint32_t)gdb_read8(str + 6) <<  0;
}

uint32_t gdb_read(const char *str) {
  uint32_t x = 0;
  for (int i = 0; str[i]; ++i)
    x = x * 16 + hex_to_int(str[i]);
  return x;
}

void gdb_write8(char *str, uint8_t x) {
  str[0] = int_to_hex(x >> 4);
  str[1] = int_to_hex(x & 0xf);
  str[2] = 0;
}

void gdb_write32(char *str, uint32_t x) {
  gdb_write8(str    , (x >>  0) & 0xff);
  gdb_write8(str + 2, (x >>  8) & 0xff);
  gdb_write8(str + 4, (x >> 16) & 0xff);
  gdb_write8(str + 6, (x >> 24) & 0xff);
}

void gdb_send_packet(FILE *out, const char *packet) {
  // DEBUG
  debug("Send: ");

  fputc('$', out);
  fputs(packet, out);
  fputc('#', out);

  // DEBUG
  printf("$%s#", packet);

  uint8_t cksum = 0;
  char c;
  for (int i = 0; (c = packet[i]); ++i)
    cksum += c;
  gdb_put8(out, cksum);
  fflush(out);

  // DEBUG
  putchar(int_to_hex(cksum >> 4));
  putchar(int_to_hex(cksum & 0xf));
  putchar('\n');
  debug("cksum: %x\n", (int)cksum);
}

void gdb_handle_supported(FILE *out, char *cmd) {
  static const char *supported_features[] = {
    "hwbreak+",
    NULL
  };

  char buf[1024];
  char *s = cmd;
  while (*s != '\0' && *s != ':')
    ++s;
  if (*s == ':')
    ++s;

  char *feature = strtok(s, ";");
  char packet[1024];
  packet[0] = 0;
  while (feature) {
    // DEBUG
    debug("feature: %s\n", feature);

    int supported = 0;
    for (int i = 0; supported_features[i]; ++i) {
      if (strcmp(supported_features[i], feature) == 0) {
        supported = 1;
        break;
      }
    }
    if (supported) {
      if (packet[0] != 0)
        strcat(packet, ";");
      strcat(packet, feature);
    }
    feature = strtok(NULL, ";");
  }

  gdb_send_packet(out, packet);
}

void gdb_handle_halt_reason(FILE *out, char *cmd) {
  gdb_send_packet(out, "S05");

}

void gdb_handle_attached(FILE *out, char *cmd) {
  gdb_send_packet(out, "1");
}

void gdb_handle_read_registers(FILE *out, char *cmd) {
  // TODO: replace this with JTAG probe
  char buf[1024];
  buf[0] = 0;
  char *p = buf;
  for (int i = 0; i < 16; ++i) {
    gdb_write32(p, 0xdeadbeef);
    p += 8;
  }
  gdb_send_packet(out, buf);
}

void gdb_handle_read_memory(FILE *out, char *cmd) {
  // DEBUG
  debug("memory command: %s\n", cmd);

  int i = 0;
  while (cmd[i] != ',')
    ++i;
  cmd[i] = 0;
  uint32_t addr = gdb_read(cmd + 1);
  uint32_t len = gdb_read(cmd + i + 1);

  // DEBUG
  debug("addr: %x, len: %d\n", addr, len);

  char buf[1024], *p = buf;
  for (int i = 0; i < len; ++i) {
    switch ((addr + i) % 4) {
      case 3:
        gdb_write8(p, 0xe0);
        break;
      case 2:
        gdb_write8(p, 0x81);
        break;
      case 1:
        gdb_write8(p, 0x00);
        break;
      case 0:
        gdb_write8(p, 0x02);
        break;
    }
    p += 2;
  }
  *p = 0;

  // TODO: replace this with JTAG probe
  gdb_send_packet(out, buf);
}

void gdb_handle_read_register(FILE *out, char *cmd) {
  uint8_t reg = gdb_read8(cmd + 1);

  // DEBUG
  debug("register: %d\n", (int)reg);

  // TODO: replace with JTAG probe
  char buf[1024];
  gdb_write32(buf, 0x00000000);
  gdb_send_packet(out, buf);
}

typedef void (*handler_t)(FILE*, char*);
const char *handler_cmds[] = {
  "qSupported",
  "?",
  "qAttached",
  "g",
  "m",
  "p",
  NULL
};
handler_t handlers[] = {
  gdb_handle_supported,
  gdb_handle_halt_reason,
  gdb_handle_attached,
  gdb_handle_read_registers,
  gdb_handle_read_memory,
  gdb_handle_read_register,
};

void gdb_handle_cmd(FILE *out, char *cmd) {
  const char *handler_cmd;
  size_t cmd_len = strlen(cmd);
  for (int i = 0; (handler_cmd = handler_cmds[i]); ++i) {
    size_t len = strlen(handler_cmd);
    if (len > cmd_len)
      len = cmd_len;
    if (strncmp(cmd, handler_cmd, len) == 0) {
      output("Info: handling %s\n", handler_cmd);
      handlers[i](out, cmd);
      return;
    }
  }

  output("Warning: unhandled command %s\n", cmd);
  gdb_send_packet(out, "");
}

void gdb_read_packet(FILE *in, FILE *out) {
  char c;
  while ((c = fgetc(in)) != '$');
  uint8_t cksum = 0;
  char cmd[256];
  int i = 0;
  while ((c = fgetc(in)) != '#') {
    cmd[i++] = c;
    cksum += c;
  }
  cmd[i] = '\0';

  uint8_t ref_cksum = gdb_get8(in);
  if (cksum == ref_cksum) {
    fputc('+', out);
    gdb_handle_cmd(out, cmd);
  } else {
    fputc('-', out);
    output("Warning: ignored packet with incorrect checksum\n");
  }
}

void gdb_relay(void) {
  int sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1)
    panic("Error: %s\n", strerror(errno));

  struct sockaddr_in server_addr;
  server_addr.sin_family = AF_INET;
  inet_aton("0.0.0.0", &server_addr.sin_addr);
  server_addr.sin_port = htons(GDB_PORT);
  if (bind(sfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
    panic("Error: %s\n", strerror(errno));

  if (listen(sfd, LISTEN_BACKLOG) == -1)
    panic("Error: %s\n", strerror(errno));

  struct sockaddr_in gdb_addr;
  socklen_t gdb_addr_len;
  int gfd = accept(sfd, (struct sockaddr *)&gdb_addr, &gdb_addr_len);
  if (gfd == -1)
    panic("Error: %s\n", strerror(errno));

  FILE *in = fdopen(dup(gfd), "rb");
  FILE *out = fdopen(dup(gfd), "wb");
  while (1)
    gdb_read_packet(in, out);

  if (fclose(in) == -1)
    panic("Error: %s\n", strerror(errno));
  if (fclose(out) == -1)
    panic("Error: %s\n", strerror(errno));
}

int main(int argc, char *argv[]) { 
  // DEBUG
  gdb_relay();
  return 0;

    char *dev_name = 0;
    char *pi_prog = 0;

    // used to pass the file descriptor to another program.
    char **exec_argv = 0;

    // a good extension challenge: tune timeout and baud rate transmission
    //
    // on linux, baud rates are defined in:
    //  /usr/include/asm-generic/termbits.h
    //
    // when I tried with sw-uart, these all worked:
    //      B9600
    //      B115200
    //      B230400
    //      B460800
    //      B576000
    // can almost do: just a weird start character.
    //      B1000000
    // all garbage?
    //      B921600
    unsigned baud_rate = B115200;
    // unsigned baud_rate = 921600;

    // by default is 0x8000
    unsigned boot_addr = ARMBASE;

    // we do manual option parsing to make things a bit more obvious.
    // you might rewrite using getopt().
    progname = argv[0];
    for(int i = 1; i < argc; i++) {
        if(strcmp(argv[i], "--trace-control") == 0)  {
            trace_p = TRACE_CONTROL_ONLY;
        } else if(strcmp(argv[i], "--trace-all") == 0)  {
            trace_p = TRACE_ALL;
        } else if(strcmp(argv[i], "--last") == 0)  {
            dev_name = find_ttyusb_last();
        } else if(strcmp(argv[i], "--first") == 0)  {
            dev_name = find_ttyusb_first();
        // we assume: anything that begins with /dev/ is a device.
        } else if(prefix_cmp(argv[i], "/dev/")) {
            dev_name = argv[i];
        // we assume: anything that ends in .bin is a pi-program.
        } else if(suffix_cmp(argv[i], ".bin")) {
            pi_prog = argv[i];
        } else if(strcmp(argv[i], "--baud") == 0) {
            i++;
            if(!argv[i])
                usage("missing argument to --baud\n");
            baud_rate = atoi(argv[i]);
        } else if(strcmp(argv[i], "--addr") == 0) {
            i++;
            if(!argv[i])
                usage("missing argument to --addr\n");
            boot_addr = atoi(argv[i]);
        } else if(strcmp(argv[i], "--exec") == 0) {
            i++;
            if(!argv[i])
                usage("missing argument to --exec\n");
            exec_argv = &argv[i];
            break;
        } else if(strcmp(argv[i], "--device") == 0) {
            i++;
            if(!argv[i])
                usage("missing argument to --device\n");
            dev_name = argv[i];
        } else {
            usage("unexpected argument=<%s>\n", argv[i]);
        }
    }
    if(!pi_prog)
        usage("no pi program\n");

    // 1. get the name of the ttyUSB.
    if(!dev_name) {
        dev_name = find_ttyusb_last();
        if(!dev_name)
            panic("didn't find a device\n");
    }
    debug_output("done with options: dev name=<%s>, pi-prog=<%s>, trace=%d\n", 
            dev_name, pi_prog, trace_p);

    if(exec_argv)
        argv_print("BOOT: --exec argv:", exec_argv);

    // 2. open the ttyUSB in 115200, 8n1 mode
    int tty = open_tty(dev_name);
    if(tty < 0)
        panic("can't open tty <%s>\n", dev_name);

    // timeout is in tenths of a second.  tuning this can speed up
    // checking.
    //
    // if you are on linux you can shrink down the <2*8> timeout
    // threshold.  if your my-install isn't reseting when used 
    // during checkig, it's likely due to this timeout being too
    // small.
    double timeout_tenths = 2*5;
    int fd = set_tty_to_8n1(tty, baud_rate, timeout_tenths);
    if(fd < 0)
        panic("could not set tty: <%s>\n", dev_name);

    // 3. read in program [probably should just make a <file_size>
    //    call and then shard out the pieces].
	unsigned nbytes;
    uint8_t *code = read_file(&nbytes, pi_prog);

    // 4. let's send it!
	debug_output("%s: tty-usb=<%s> program=<%s>: about to boot\n", 
                progname, dev_name, pi_prog);
    simple_boot(fd, boot_addr, code, nbytes);

    // 5. echo output from pi
    if(!exec_argv)
        pi_echo(0, fd, dev_name);
    else {
        todo("not handling exec_argv");
        handoff_to(fd, TRACE_FD, exec_argv);
    }
	return 0;
}
