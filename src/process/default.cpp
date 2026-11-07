/* What every lemming does unless a newer handler takes the message first. */
#include "lemming.h"

/* 0x10001bb0 */
int DefaultHandler(int msg, int a, int b, LEMMING *l)
{
    switch (msg) {
    case MSG_WALL:
        l->dir = -l->dir;
        return 1;
    case MSG_FALL:
        l->action->Leave(l);
        g_faller.Enter(l);
        return 1;
    case MSG_LAND:
        if (l->count > 60) {
            l->action->Leave(l);
            g_splatter.Enter(l);
        } else {
            l->action->Leave(l);
            g_walker.Enter(l);
        }
        return 1;
    case MSG_OFFTOP:
    case MSG_OFFBOTTOM:
        l->action->Leave(l);
        l->state |= LS_GONE;
        return 1;
    case MSG_OFFLEFT:
    case MSG_OFFRIGHT:
        l->dir = -l->dir;
        return 1;
    case MSG_OBJECT:
        switch (a & 0xff) {
        case OBJ_NONE:
            return 1;
        case OBJ_EXIT:
            l->action->Leave(l);
            g_homer.Enter(l);
            return 1;
        case OBJ_TURNLEFT:
            l->dir = -1;
            return 1;
        case OBJ_TURNRIGHT:
            l->dir = 1;
            return 1;
        case OBJ_TRAP:
            if (TriggerObject(a >> 8)) {
                l->action->Leave(l);
                l->state |= LS_GONE;
            }
            return 1;
        case OBJ_WATER:
            l->action->Leave(l);
            g_drowner.Enter(l);
            return 1;
        case OBJ_FIRE:
            l->action->Leave(l);
            g_frier.Enter(l);
            return 1;
        case OBJ_ONEWAYLEFT:
            return 1;
        case OBJ_ONEWAYRIGHT:
            return 1;
        case OBJ_STEEL:
            return 1;
        case OBJ_BLOCKER:
            return 1;
        }
        break;
    case MSG_COUNTDOWN:
        l->action->Leave(l);
        g_flapper.Enter(l);
        return 1;
    }
    return 0;
}

/* 0x10001de0 */
int CanAssign(LEMMING *l, int type)
{
    return l->type & type;
}
