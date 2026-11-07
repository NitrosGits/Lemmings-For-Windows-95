/*
 * Scripts (not used by the game): lines of frame:lemming:action, giving
 * that lemming that skill on that frame.
 */
/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_top0, pad_top1, pad_top2, pad_top3, pad_top4, pad_top5, pad_top6, pad_top7, pad_top8, pad_top9, pad_top10, pad_top11, pad_top12;

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lemming.h"

struct SCRIPT {
    int frame;
    int lemming;
    Action *action;
};

char *g_actionNames[NUM_ACTIONS] = {
    "Walker", "Faller", "Climber", "Flipper", "Floater", "Flapper", "Exploder", "Splatter", "Blocker",
    "Drowner", "Frier", "Homer", "Builder", "Shrugger", "Digger", "Basher", "Miner",
};
Action *g_actions[NUM_ACTIONS] = {
    &g_walker, &g_faller, &g_climber, &g_flipper, &g_floater, &g_flapper, &g_exploder, &g_splatter,
    &g_blocker, &g_drowner, &g_frier, &g_homer, &g_builder, &g_shrugger, &g_digger, &g_basher, &g_miner,
};
int g_scriptIndex = 0;
int g_scriptCount = 0;

char g_token[256];
SCRIPT *g_script[200];

/* 0x10003f20: the text up to the delimiter or the end of the line */
static char *ReadToken(FILE *f, char delim)
{
    int done = 0;
    int i = 0;
    char c;

    do {
        if (feof(f)) {
            done = 1;
        } else {
            c = (char)fgetc(f);
            if (c == delim || c == '\n' || c == '\r') {
                done = 1;
                g_token[i] = 0;
            } else if (i < 256) {
                g_token[i] = c;
            }
        }
        i++;
    } while (!done);
    return g_token;
}

/* 0x10003f80 */
void WINAPI InitScript(char *file)
{
    FILE *f;
    char buf[128];
    int frame, lemming;
    int i;

    if ((f = fopen(file, "rt")) == NULL) {
        wsprintf(buf, "ERROR : Unable to open script file %s\n", file);
        MessageBox(NULL, buf, "ERROR: Process Module", MB_OK);
        return;
    }
    while (!feof(f)) {
        frame = atoi(ReadToken(f, ':'));
        lemming = atoi(ReadToken(f, ':'));
        strcpy(buf, ReadToken(f, ':'));
        for (i = 0; i < NUM_ACTIONS; i++) {
            if (strcmp(buf, g_actionNames[i]) == 0) {
                g_script[g_scriptCount] = new SCRIPT;
                g_script[g_scriptCount]->frame = frame;
                g_script[g_scriptCount]->lemming = lemming;
                g_script[g_scriptCount]->action = g_actions[i];
                g_scriptCount++;
            }
        }
    }
    fclose(f);
    g_scriptIndex = 0;
}

/* Symbol padding (see docs/DECOMP.md): the original had more declarations here. */
extern int pad_checkscript0, pad_checkscript1, pad_checkscript2, pad_checkscript3, pad_checkscript4, pad_checkscript5, pad_checkscript6, pad_checkscript7, pad_checkscript8, pad_checkscript9, pad_checkscript10, pad_checkscript11, pad_checkscript12, pad_checkscript13, pad_checkscript14, pad_checkscript15, pad_checkscript16, pad_checkscript17, pad_checkscript18, pad_checkscript19, pad_checkscript20, pad_checkscript21;

/* 0x10004100 */
void WINAPI CheckScript(int frame)
{
    if (g_scriptCount != g_scriptIndex && g_script[g_scriptIndex]->frame == frame) {
        LEMMING *l = g_lemmings[g_script[g_scriptIndex]->lemming];
        g_script[g_scriptIndex]->action->Assign(l);
        g_scriptIndex++;
    }
}
