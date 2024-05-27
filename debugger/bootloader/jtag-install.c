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
#include <assert.h>
#include <ctype.h>
#include <unistd.h>
#include "libunix.h"
#include "put-code.h"
#include "boot.h"


#if 0
// hack-y state machine to indicate when we've seen the special string
// 'DONE!!!' from the pi telling us to shutdown.
int boot_done(unsigned char *s) {
    static unsigned pos = 0;
    const char exit_string[] = "DONE!!!\n";
    const int n = sizeof exit_string - 1;

    for(; *s; s++) {
        assert(pos < n);
        if(*s != exit_string[pos++]) {
            pos = 0;
            return boot_done(s+1); // check remainder
        }
        // maybe should check if "DONE!!!" is last thing printed?
        if(pos == sizeof exit_string - 1)
            return 1;
    }
    return 0;
}

// overwrite any unprintable characters with a space.
// otherwise terminals can go haywire/bizarro.
// note, the string can contain 0's, so we send the
// size.
void remove_nonprint(uint8_t *buf, int n) {
    for(int i = 0; i < n; i++) {
        uint8_t *p = &buf[i];
        if(isprint(*p) || (isspace(*p) && *p != '\r'))
            continue;
        *p = ' ';
    }
}

// read and echo the characters from the usbtty until it closes 
// (pi rebooted) or we see a string indicating a clean shutdown.
void boot_echo(int unix_fd, int pi_fd, const char *portname) {
  enum {
    STATE_ECHO,
    STATE_OP,
    STATE_,
  };

  static int pkt_hdr_cnt = 0, state = STATE_ECHO;

    assert(pi_fd);
    /*
    if(portname)
        output("listening on ttyusb=<%s>\n", portname);
    */

    while(1) {
        unsigned char buf[4096];

        int n;
        if((n=read_timeout(unix_fd, buf, sizeof buf, 1000))) {
            buf[n] = 0;
            // output("about to echo <%s> to pi\n", buf);
            write_exact(pi_fd, buf, n);
        }

        if(!can_read_timeout(pi_fd, 1000))
            continue;
        n = read(pi_fd, buf, sizeof buf - 1);

        if(!n) {
            // this isn't the program's fault.  so we exit(0).
            if(!portname || tty_gone(portname))
                clean_exit("pi ttyusb connection closed.  cleaning up\n");
            // so we don't keep banginging on the CPU.
            usleep(1000);
        } else if(n < 0) {
            sys_die(read, "pi connection closed.  cleaning up\n");
        } else {
            buf[n] = 0;
            // if you keep getting "" "" "" it's b/c of the GET_CODE message from bootloader
            // DEBUG
            // remove_nonprint(buf,n);
            // output("%s", buf);
            for (int i = 0; i < n; ++i) {
              char c = buf[i];
              switch (state) {
                case STATE_ECHO:
                  if (c == BOOT_PKT_HDR) {
                    if (++pkt_hdr_cnt == BOOT_PKT_HDR_LEN)
                      state = STATE_READ_PKT;
                  } else {
                    while (--pkt_hdr_cnt)
                      uart_put8(c);
                  }
                  break;
                case STATE_READ_PKT:
                  break;
              }
              if (reading_pkt) {
              } else {
              }
            }

            if(boot_done(buf)) {
                // output("\nSaw done\n");
                clean_exit("\nbootloader: pi exited.  cleaning up\n");
            }
        }
    }
    notreached();
}
#endif


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

typedef struct {
  FILE *gdb_in;
  FILE *gdb_out;
  FILE *pi_in;
  FILE *pi_out;
} com_t;

void pi_send8(FILE *out, uint8_t x) {
  fputc(x, out);
}

void pi_send32(FILE *out, uint32_t x) {
  pi_send8(out, (x >> 24) & 0xff);
  pi_send8(out, (x >> 16) & 0xff);
  pi_send8(out, (x >>  8) & 0xff);
  pi_send8(out,  x        & 0xff);
}

uint8_t pi_read8(FILE *in) {
  return fgetc(in);
}

uint32_t pi_read32(FILE *in) {
  return (uint32_t)pi_read8(in) << 24 |
         (uint32_t)pi_read8(in) << 16 |
         (uint32_t)pi_read8(in) <<  8 |
         (uint32_t)pi_read8(in);
}

uint32_t pi_read_register(com_t *com, uint8_t r) {
  pi_send8(com->pi_out, CMD_READ_REG);
  pi_send8(com->pi_out, r);
  return pi_read32(com->pi_in);
}

uint8_t pi_read_memory(com_t *com, uint32_t addr) {
  pi_send8(com->pi_out, CMD_READ_MEM);
  pi_send32(com->pi_out, addr);
  return pi_read8(com->pi_in);
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

void gdb_handle_supported(com_t *com, char *cmd) {
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

  gdb_send_packet(com->gdb_out, packet);
}

void gdb_handle_halt_reason(com_t *com, char *cmd) {
  gdb_send_packet(com->gdb_out, "S05");
}

void gdb_handle_attached(com_t *com, char *cmd) {
  gdb_send_packet(com->gdb_out, "1");
}

void gdb_handle_read_registers(com_t *com, char *cmd) {
  char buf[1024], *s = buf;
  for (uint8_t r = 0; r < 16; ++r) {
    uint32_t x = pi_read_register(com, r);
    gdb_write32(s, x);
    s += 8;
  }
  gdb_send_packet(com->gdb_out, buf);
}

void gdb_handle_read_memory(com_t *com, char *cmd) {
  // DEBUG
  debug("memory command: %s\n", cmd);

  int i = 0;
  while (cmd[i] != ',')
    ++i;
  cmd[i] = 0;
  uint32_t addr = gdb_read(cmd + 1);
  uint32_t len = gdb_read(cmd + i + 1);

  char buf[1024], *s = buf;
  for (uint32_t i = addr; i < addr + len; ++i) {
    // DEBUG
    debug("reading address %x\n", i);

    uint8_t x = pi_read_memory(com, i);
    gdb_write8(s, x);
    s += 2;
  }

  gdb_send_packet(com->gdb_out, buf);
}

void gdb_handle_read_register(com_t *com, char *cmd) {
  uint8_t r = gdb_read8(cmd + 1);

  char buf[1024];
  uint32_t x = pi_read_register(com, r);
  gdb_write32(buf, x);

  gdb_send_packet(com->gdb_out, buf);
}

void gdb_handle_detach(com_t *com, char *cmd) {
  pi_send8(com->pi_out, CMD_DETACH);
  gdb_send_packet(com->gdb_out, "OK");
  exit(0);
}

void gdb_handle_step(com_t *com, char *cmd) {
  // TODO: handle step with target address
  pi_send8(com->pi_out, CMD_STEP);
  pi_read8(com->pi_in);
  gdb_handle_halt_reason(com, cmd);
}

typedef void (*handler_t)(com_t*, char*);
const char *handler_cmds[] = {
  "qSupported",
  "?",
  "qAttached",
  "g",
  "m",
  "p",
  "D",
  "s",
  NULL
};
handler_t handlers[] = {
  gdb_handle_supported,
  gdb_handle_halt_reason,
  gdb_handle_attached,
  gdb_handle_read_registers,
  gdb_handle_read_memory,
  gdb_handle_read_register,
  gdb_handle_detach,
  gdb_handle_step
};

void gdb_handle_cmd(com_t *com, char *cmd) {
  const char *handler_cmd;
  size_t cmd_len = strlen(cmd);
  for (int i = 0; (handler_cmd = handler_cmds[i]); ++i) {
    size_t len = strlen(handler_cmd);
    if (len > cmd_len)
      len = cmd_len;
    if (strncmp(cmd, handler_cmd, len) == 0) {
      output("Info: handling %s\n", handler_cmd);
      handlers[i](com, cmd);
      return;
    }
  }

  output("Warning: unhandled command %s\n", cmd);
  gdb_send_packet(com->gdb_out, "");
}

void gdb_read_packet(com_t *com) {
  char c;
  while ((c = fgetc(com->gdb_in)) != '$');
  uint8_t cksum = 0;
  char cmd[256];
  int i = 0;
  while ((c = fgetc(com->gdb_in)) != '#') {
    cmd[i++] = c;
    cksum += c;
  }
  cmd[i] = '\0';

  uint8_t ref_cksum = gdb_get8(com->gdb_in);
  if (cksum == ref_cksum) {
    fputc('+', com->gdb_out);
    gdb_handle_cmd(com, cmd);
  } else {
    fputc('-', com->gdb_out);
    output("Warning: ignored packet with incorrect checksum\n");
  }
}

void gdb_relay(int pi_fd) {
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

  FILE *gdb_in = fdopen(dup(gfd), "rb");
  FILE *gdb_out = fdopen(dup(gfd), "wb");
  FILE *pi_in = fdopen(dup(pi_fd), "rb");
  FILE *pi_out = fdopen(dup(pi_fd), "wb");
  com_t com = (com_t){
    .gdb_in = gdb_in,
    .gdb_out = gdb_out,
    .pi_in = pi_in,
    .pi_out = pi_out,
  };
  while (1)
    gdb_read_packet(&com);

  if (fclose(gdb_in) == -1)
    panic("Error: %s\n", strerror(errno));
  if (fclose(gdb_out) == -1)
    panic("Error: %s\n", strerror(errno));
  if (fclose(pi_in) == -1)
    panic("Error: %s\n", strerror(errno));
  if (fclose(pi_out) == -1)
    panic("Error: %s\n", strerror(errno));
}

int main(int argc, char *argv[]) { 
  // DEBUG
  // gdb_relay();
  // return 0;

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
        gdb_relay(fd);
    else {
        todo("not handling exec_argv");
        handoff_to(fd, TRACE_FD, exec_argv);
    }
	return 0;
}
