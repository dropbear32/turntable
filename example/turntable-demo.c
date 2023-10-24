/*
    turntable - client-to-server audio streaming
    Copyright (c) 2021-2022 Ally Sommers

    This code is licensed under the BSD 3-Clause License. A copy of this
    license is included with this code.
*/

#include "../src/turntable.h"
#include "../src/util/pthread.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define INPUT_MAX_LEN 256
#define INPUT_PROMPT  "turntable> "

void cmd_client(char *);
void cmd_quit(char *);
void cmd_help(char *);
void cmd_pause(char *);
void cmd_skip(char *);
void cmd_play(char *);
void cmd_quit(char *);
void cmd_resume(char *);
void cmd_server(char *);
void cmd_disconnect(char *);

struct cmd_entry {
    char *cmd;
    void (*func)(char *);
    char *description;
};

// clang-format off
struct cmd_entry const CMD_TABLE[] = {
    { "client",     cmd_client,     "connects to a server with the specified hostname" },
    { "disconnect", cmd_disconnect, "closes the server / disconnects from a server" },
    { "exit",       cmd_quit,       "exits the program" },
    { "help",       cmd_help,       "prints this help" },
    { "pause",      cmd_pause,      "pauses the currently playing song" },
    { "skip",       cmd_skip,       "skips the currently playing song" },
    { "play",       cmd_play,       "plays the passed file" },
    { "quit",       cmd_quit,       "exits the program" },
    { "resume",     cmd_resume,     "resumes playback of the current song" },
    { "server",     cmd_server,     "starts a server" },
};
// clang-format on

void cmd_client(char *input) {
    if (strlen(input) < 2) {
        puts("syntax: client [hostname]");
        return;
    }
    turntable_init_client(input + 1);
}

void cmd_server(char *input) {
    (void)input;
    turntable_init_server();
}

void cmd_resume(char *input) {
    if (strlen(input) > 1) {
        puts("syntax: resume");
        return;
    }
    puts("Resuming...");
    turntable_resume_audio();
}

void cmd_pause(char *input) {
    if (strlen(input) > 1) {
        puts("syntax: pause");
        return;
    }
    puts("Pausing...");
    turntable_pause_audio();
}

void cmd_skip(char *input) {
    if (strlen(input) > 1) {
        puts("syntax: skip");
        return;
    }
    puts("Skipping...");
    turntable_skip_audio();
}

void cmd_play(char *input) {
    if (strlen(input) < 2) {
        puts("syntax: play [file]");
        return;
    }

    turntable_queue_file(input + 1);
}

void cmd_disconnect(char *input) {
    if (strlen(input) > 1) {
        puts("syntax: disconnect");
        return;
    }
    if (turntable_connected_as_client()) {
        turntable_close_client();
    } else if (turntable_connected_as_server()) {
        turntable_close_server();
    }
}

void cmd_quit(char *input) {
    if (strlen(input) > 1) {
        puts("syntax: quit");
        return;
    }
    cmd_disconnect(input);
    exit(0);
}

void cmd_help(char *input) {
    if (strlen(input) > 1) {
        puts("Command-specific help coming soon! For now, please enjoy the default help:");
    }
    for (size_t i = 0; i < (sizeof(CMD_TABLE) / sizeof(CMD_TABLE[0])); i++) {
        printf("%-12s: %s\n", CMD_TABLE[i].cmd, CMD_TABLE[i].description);
    }
}

void newprompt(bool newline) {
    printf(newline ? "\n" INPUT_PROMPT : INPUT_PROMPT);
}

int main(void) {
    char *input_buffer = malloc(INPUT_MAX_LEN);
    size_t input_len;
    char *first_space;

    turntable_init();

    while (true) {
        newprompt(true);
        memset(input_buffer, 0, INPUT_MAX_LEN);
        fgets(input_buffer, INPUT_MAX_LEN, stdin);
        input_len = strlen(input_buffer);

        // We don't need a trailing newline, so drop it if it exists
        if (input_buffer[input_len - 1] == '\n') {
            input_buffer[input_len - 1] = '\0';
        }

        if (strlen(input_buffer) == 0) {
            continue;
        } else {
            for (size_t i = 0; i < (sizeof(CMD_TABLE) / sizeof(CMD_TABLE[0])); i++) {
                if (strstr(input_buffer, CMD_TABLE[i].cmd) == input_buffer) {
                    CMD_TABLE[i].func(input_buffer + strlen(CMD_TABLE[i].cmd));
                    goto match_found;
                }
            }
        }

        // If there's a space in the input, make it the end
        // to just print the first word of the input
        first_space = strchr(input_buffer, ' ');
        if (first_space != NULL) {
            *first_space = '\0';
        }

        printf("Unknown command '%s'\n", input_buffer);
    match_found:;
    }
    free(input_buffer);
}
