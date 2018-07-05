#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "protos.h"

#define INQ_SHOUT 1
#define INQ_LOOSE 0

#define SWORD_ANCIENTS 25000
/*
 *  list of room #s
 */
#define Elf_Home     1414
#define Bakery       3009
#define Dump         3030
#define Ivory_Gate    1499

/*
 * external vars
 */

extern struct room_data *world;
extern struct char_data *character_list;
extern struct descriptor_data *descriptor_list;
extern struct index_data *obj_index;
extern struct time_info_data time_info;
extern struct index_data *mob_index;
extern struct weather_data weather_info;
extern int      top_of_world;
extern struct int_app_type int_app[26];
extern int      RacialMax[][MAX_CLASS];

extern struct title_type titles[MAX_CLASS][ABS_MAX_LVL];
extern struct spell_info_type spell_info[];
extern int      spell_index[MAX_SPL_LIST];
extern char    *dirs[];

extern int      drink_aff[][3];
extern struct weather_data weather_info;


/*
 * Data declarations
 */

struct social_type {
    char           *cmd;
    int             next_line;
};

struct memory {
    short           pointer;
    char          **names;
    int            *status;
    short           index;
    short           c;
};


int affect_status(struct memory *mem, struct char_data *ch,
                  struct char_data *t, int aff_status);


/*************************************/
/*
 * predicates for find_path function
 */

int is_target_room_p(int room, void *tgt_room)
{
    return room == (int) tgt_room;
}

int named_object_on_ground(int room, void *c_data)
{
    char           *name = c_data;
    return 0 != get_obj_in_list(name, real_roomp(room)->contents);
}

/*
 * predicates for find_path function
 */
/*************************************/

/*
 ********************************************************************
 *  Special procedures for rooms                                    *
 ******************************************************************** */

struct char_data *FindMobInRoomWithFunction(int room, int (*func) ())
{
    struct char_data *temp_char,
                   *targ;

    targ = 0;

    if (room > NOWHERE) {
        for (temp_char = real_roomp(room)->people; (!targ) && (temp_char);
             temp_char = temp_char->next_in_room) {
            if (IS_MOB(temp_char) && mob_index[temp_char->nr].func == func) {
                targ = temp_char;
            }
        }
    } else {
        return (NULL);
    }
    return (targ);
}


int dump(struct char_data *ch, int cmd, char *arg, struct room_data *rp,
         int type)
{
    struct obj_data *k;
    char            buf[100];
    struct char_data *tmp_char;
    int             value = 0;

    void            do_drop(struct char_data *ch, char *argument, int cmd);
    char           *fname(char *namelist);

    for (k = real_roomp(ch->in_room)->contents; k;
         k = real_roomp(ch->in_room)->contents) {
        sprintf(buf, "The %s vanish in a puff of smoke.\n\r",
                fname(k->name));
        for (tmp_char = real_roomp(ch->in_room)->people; tmp_char;
             tmp_char = tmp_char->next_in_room) {
            if (CAN_SEE_OBJ(tmp_char, k)) {
                send_to_char(buf, tmp_char);
            }
        }
        extract_obj(k);
    }

    if (cmd != 60) {
        return (FALSE);
    }
    do_drop(ch, arg, cmd);

    value = 0;

    for (k = real_roomp(ch->in_room)->contents; k;
         k = real_roomp(ch->in_room)->contents) {
        sprintf(buf, "The %s vanish in a puff of smoke.\n\r", fname(k->name));
        for (tmp_char = real_roomp(ch->in_room)->people; tmp_char;
             tmp_char = tmp_char->next_in_room) {
            if (CAN_SEE_OBJ(tmp_char, k)) {
                send_to_char(buf, tmp_char);
            }
        }
        value += (MIN(1000, MAX(k->obj_flags.cost / 4, 1)));
        extract_obj(k);
    }

    if (value) {
        act("You are awarded for outstanding performance.",
            FALSE, ch, 0, 0, TO_CHAR);
        act("$n has been awarded for being a good citizen.",
            TRUE, ch, 0, 0, TO_ROOM);

        if (GetMaxLevel(ch) < 3) {
            gain_exp(ch, MIN(100, value));
        } else {
            GET_GOLD(ch) += value;
        }
    }
    return( TRUE );
}

int mayor(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
          int type)
{
    static char     open_path[] =
        "W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

    static char     close_path[] =
        "W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static char    *path;
    static int      index;
    static bool     move = FALSE;

    if (type == EVENT_WINTER) {
        GET_POS(ch) = POSITION_STANDING;
        do_shout(ch, "Aieee!   The rats!  The rats are coming!  Aieeee!", 0);
        return (TRUE);
    }

    if (!move) {
        if (time_info.hours == 6) {
            move = TRUE;
            path = open_path;
            index = 0;
        } else if (time_info.hours == 20) {
            move = TRUE;
            path = close_path;
            index = 0;
        }
    }

    if (cmd || !move || GET_POS(ch) < POSITION_SLEEPING ||
        GET_POS(ch) == POSITION_FIGHTING) {
        if (!ch->specials.fighting) {
            return (FALSE);
        }
        return (fighter(ch, cmd, arg, mob, type));
    }

    switch (path[index]) {
    case '0':
    case '1':
    case '2':
    case '3':
        do_move(ch, "", path[index] - '0' + 1);
        break;

    case 'W':
        GET_POS(ch) = POSITION_STANDING;
        act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
        break;

    case 'S':
        GET_POS(ch) = POSITION_SLEEPING;
        act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0,
            TO_ROOM);
        break;

    case 'a':
        if (check_soundproof(ch)) {
            return (FALSE);
        }
        act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
        act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
        break;

    case 'b':
        if (check_soundproof(ch)) {
            return (FALSE);
        }
        act("$n says 'What a view! I must get something done about that dump!'",
            FALSE, ch, 0, 0, TO_ROOM);
        break;

    case 'c':
        if (check_soundproof(ch)) {
            return (FALSE);
        }
        act("$n says 'Vandals! Youngsters nowadays have no respect for "
            "anything!'", FALSE, ch, 0, 0, TO_ROOM);
        break;

    case 'd':
        if (check_soundproof(ch)) {
            return (FALSE);
        }
        act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
        break;

    case 'e':
        if (check_soundproof(ch)) {
            return (FALSE);
        }
        act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0,
            TO_ROOM);
        break;

    case 'E':
        if (check_soundproof(ch)) {
            return (FALSE);
        }
        act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0,
            TO_ROOM);
        break;

    case 'O':
        do_unlock(ch, "gate", 0);
        do_open(ch, "gate", 0);
        break;

    case 'C':
        do_close(ch, "gate", 0);
        do_lock(ch, "gate", 0);
        break;

    case '.':
        move = FALSE;
        break;

    }

    index++;
    return FALSE;
}

struct char_data *find_mobile_here_with_spec_proc(int (*fcn) (), int rnumber)
{
    struct char_data *temp_char;

    for (temp_char = real_roomp(rnumber)->people; temp_char;
         temp_char = temp_char->next_in_room) {
        if (IS_MOB(temp_char) && mob_index[temp_char->nr].func == fcn) {
            return temp_char;
        }
    }
    return NULL;
}

/*
 *******************************************************************
 *  General special procedures for mobiles                         *
 *******************************************************************
 */

/*
 * SOCIAL GENERAL PROCEDURES
 *
 * If first letter of the command is '!' this will mean that the following
 * command will be executed immediately.
 *
 * "G",n : Sets next line to n "g",n : Sets next line relative to n, fx.
 * line+=n "m<dir>",n : move to <dir>, <dir> is 0,1,2,3,4 or 5 "w",n : Wake
 * up and set standing (if possible) "c<txt>",n : Look for a person named
 * <txt> in the room "o<txt>",n : Look for an object named <txt> in the
 * room "r<int>",n : Test if the npc in room number <int>? "s",n : Go to
 * sleep, return false if can't go sleep "e<txt>",n : echo <txt> to the
 * room, can use $o/$p/$N depending on contents of the **thing "E<txt>",n :
 * Send <txt> to person pointed to by thing "B<txt>",n : Send <txt> to
 * room, except to thing "?<num>",n : <num> in [1..99]. A random chance of
 * <num>% success rate. Will as usual advance one line upon sucess, and
 * change relative n lines upon failure. "O<txt>",n : Open <txt> if in
 * sight. "C<txt>",n : Close <txt> if in sight. "L<txt>",n : Lock <txt> if
 * in sight. "U<txt>",n : Unlock <txt> if in sight.
 */

/*
 * Execute a social command.
 */
void exec_social(struct char_data *npc, char *cmd, int next_line,
                 int *cur_line, void **thing)
{
    bool            ok;


    if (GET_POS(npc) == POSITION_FIGHTING) {
        return;
    }
    ok = TRUE;

    switch (*cmd) {

    case 'G':
        *cur_line = next_line;
        return;

    case 'g':
        *cur_line += next_line;
        return;

    case 'e':
        act(cmd + 1, FALSE, npc, *thing, *thing, TO_ROOM);
        break;

    case 'E':
        act(cmd + 1, FALSE, npc, 0, *thing, TO_VICT);
        break;

    case 'B':
        act(cmd + 1, FALSE, npc, 0, *thing, TO_NOTVICT);
        break;

    case 'm':
        do_move(npc, "", *(cmd + 1) - '0' + 1);
        break;

    case 'w':
        if (GET_POS(npc) != POSITION_SLEEPING) {
            ok = FALSE;
        } else {
            GET_POS(npc) = POSITION_STANDING;
        }
        break;

    case 's':
        if (GET_POS(npc) <= POSITION_SLEEPING) {
            ok = FALSE;
        } else {
            GET_POS(npc) = POSITION_SLEEPING;
        }
        break;

    case 'c':
        /*
         * Find char in room
         */
        *thing = get_char_room_vis(npc, cmd + 1);
        ok = (*thing != 0);
        break;

    case 'o':
        /*
         * Find object in room
         */
        *thing = get_obj_in_list_vis(npc, cmd + 1,
                                     real_roomp(npc->in_room)->contents);
        ok = (*thing != 0);
        break;

    case 'r':
        /*
         * Test if in a certain room
         */
        ok = (npc->in_room == atoi(cmd + 1));
        break;

    case 'O':
        /*
         * Open something
         */
        do_open(npc, cmd + 1, 0);
        break;

    case 'C':
        /*
         * Close something
         */
        do_close(npc, cmd + 1, 0);
        break;

    case 'L':
        /*
         * Lock something
         */
        do_lock(npc, cmd + 1, 0);
        break;

    case 'U':
        /*
         * UnLock something
         */
        do_unlock(npc, cmd + 1, 0);
        break;

    case '?':
        /*
         * Test a random number
         */
        if (atoi(cmd + 1) <= number(1, 100)) {
            ok = FALSE;
        }
        break;

    default:
        break;
    }

    if (ok) {
        (*cur_line)++;
    } else {
        (*cur_line) += next_line;
    }
}

void npc_steal(struct char_data *ch, struct char_data *victim)
{
    int             gold;

    if (IS_NPC(victim) || GetMaxLevel(victim) > MAX_MORT) {
        return;
    }
    if (AWAKE(victim) && !number(0, GetMaxLevel(ch))) {
        act("You discover that $n has $s hands in your wallet.", FALSE, ch,
            0, victim, TO_VICT);
        act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
    } else {
        /*
         * Steal some gold coins
         */
        gold = (int) ((GET_GOLD(victim) * number(1, 10)) / 100);
        if (gold > 0) {
            GET_GOLD(ch) += gold;
            GET_GOLD(victim) -= gold;
        }
    }
}

int snake(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
          int type)
{

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (GET_POS(ch) != POSITION_FIGHTING) {
        return (FALSE);
    }
    if (ch->specials.fighting &&
        ch->specials.fighting->in_room == ch->in_room) {
        act("$n poisons $N!", 1, ch, 0, ch->specials.fighting, TO_NOTVICT);
        act("$n poisons you!", 1, ch, 0, ch->specials.fighting, TO_VICT);
        cast_poison(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                    ch->specials.fighting, 0);
        return TRUE;
    }
    return FALSE;
}

int PaladinGuildGuard(struct char_data *ch, int cmd, char *arg,
                      struct char_data *mob, int type)
{
    if (!AWAKE(ch)) {
        return (FALSE);
    }
    if (!cmd) {
        if (ch->specials.fighting) {
            fighter(ch, cmd, arg, mob, type);
        }
    } else if (cmd >= 1 && cmd <= 6) {
        if ((cmd == 2) || (cmd == 1) || (cmd == 4)) {
            return (FALSE);
        }
        if (!HasClass(ch, CLASS_PALADIN)) {
            send_to_char("The guard shakes his head, and blocks your way.\n\r",
                         ch);
            act("The guard shakes his head, and blocks $n's way.", TRUE,
                ch, 0, 0, TO_ROOM);
            return (TRUE);
        }
    }
    return (FALSE);
}

#if 0

int GameGuard(struct char_data *ch, int cmd, char *arg,
              struct char_data *mob, int type)
{

    if (!cmd) {
        if (ch->specials.fighting) {
            fighter(ch, cmd, arg, mob, type);
        }
    }

    if (cmd == 4) {             /* West is field */
        if ((IS_AFFECTED(ch, AFF_TEAM_GREY)) ||
            (IS_AFFECTED(ch, AFF_TEAM_AMBER))) {
            send_to_char("The guard wishes you good luck on the field.\n\r",
                         ch);
            return (FALSE);
        } else {
            send_to_char("The guard shakes his head, and blocks your way.\n\r",
                         ch);
            act("The guard shakes his head, and blocks $n's way.", TRUE,
                ch, 0, 0, TO_ROOM);
            send_to_char("The guard says 'Your not a player! You can't enter "
                         "the field!'.\n\r", ch);
            return (TRUE);
        }
        return (FALSE);
    } else
        return (FALSE);
}

int GreyParamedic(struct char_data *ch, int cmd, char *arg,
                  struct char_data *mob, int types)
{
    struct char_data *vict,
                   *most_hurt;

    if (!cmd) {
        if (ch->specials.fighting) {
            return (cleric(ch, 0, "", mob, types));
        } else {
            if (GET_POS(ch) == POSITION_STANDING) {

                /*
                 * Find a dude to do good things upon !
                 */

                most_hurt = real_roomp(ch->in_room)->people;
                for (vict = real_roomp(ch->in_room)->people; vict;
                     vict = vict->next_in_room) {
                    if (((float) GET_HIT(vict) / (float) hit_limit(vict) <
                         (float) GET_HIT(most_hurt) /
                         (float) hit_limit(most_hurt))
                        && (CAN_SEE(ch, vict)))
                        most_hurt = vict;
                }
                if (!most_hurt)
                    return (FALSE);     /* nobody here */
                if (IS_AFFECTED(most_hurt, AFF_TEAM_GREY)) {

                    if ((float) GET_HIT(most_hurt) /
                        (float) hit_limit(most_hurt) > 0.66) {
                        if (number(0, 5) == 0) {
                            act("$n shrugs helplessly.", 1, ch, 0, 0,
                                TO_ROOM);
                        }
                        return TRUE;    /* not hurt enough */
                    }

                    if (!check_soundproof(ch)) {
                        if (number(0, 4) == 0) {
                            if (most_hurt != ch) {
                                act("$n looks at $N.", 1, ch, 0, most_hurt,
                                    TO_NOTVICT);
                                act("$n looks at you.", 1, ch, 0,
                                    most_hurt, TO_VICT);
                            }

                            if (check_nomagic(ch, 0, 0))
                                return (TRUE);

                            act("$n utters the words 'judicandus dies'.",
                                1, ch, 0, 0, TO_ROOM);
                            cast_cure_light(GetMaxLevel(ch), ch, "",
                                            SPELL_TYPE_SPELL, most_hurt,
                                            0);
                            return (TRUE);
                        }
                    }
                } else {        /* Other Team? */
                    if (IS_AFFECTED(most_hurt, AFF_TEAM_AMBER)) {

                        if (check_nomagic(ch, 0, 0))
                            return (TRUE);

                        act("$n utters the words 'die punk'.",
                            1, ch, 0, 0, TO_ROOM);
                        cast_cause_light(GetMaxLevel(ch), ch, "",
                                         SPELL_TYPE_SPELL, most_hurt, 0);
                        return (TRUE);
                    } else
                        return (FALSE);
                }
            } else {            /* I'm asleep or sitting */
                return (FALSE);
            }
        }
    }
    return (FALSE);
}

int AmberParamedic(struct char_data *ch, int cmd, char *arg,
                   struct char_data *mob, int type)
{
    struct char_data *vict,
                   *most_hurt;

    if (!cmd) {
        if (ch->specials.fighting) {
            return (cleric(ch, 0, "", mob, type));
        } else {
            if (GET_POS(ch) == POSITION_STANDING) {

                /*
                 * Find a dude to do good things upon !
                 */

                most_hurt = real_roomp(ch->in_room)->people;
                for (vict = real_roomp(ch->in_room)->people; vict;
                     vict = vict->next_in_room) {
                    if (((float) GET_HIT(vict) / (float) hit_limit(vict) <
                         (float) GET_HIT(most_hurt) /
                         (float) hit_limit(most_hurt))
                        && (CAN_SEE(ch, vict)))
                        most_hurt = vict;
                }
                if (!most_hurt)
                    return (FALSE);     /* nobody here */
                if (IS_AFFECTED(most_hurt, AFF_TEAM_AMBER)) {

                    if ((float) GET_HIT(most_hurt) /
                        (float) hit_limit(most_hurt) > 0.66) {
                        if (number(0, 5) == 0) {
                            act("$n shrugs helplessly.", 1, ch, 0, 0,
                                TO_ROOM);
                        }
                        return TRUE;    /* not hurt enough */
                    }

                    if (!check_soundproof(ch)) {
                        if (number(0, 4) == 0) {
                            if (most_hurt != ch) {
                                act("$n looks at $N.", 1, ch, 0, most_hurt,
                                    TO_NOTVICT);
                                act("$n looks at you.", 1, ch, 0,
                                    most_hurt, TO_VICT);
                            }

                            if (check_nomagic(ch, 0, 0))
                                return (TRUE);

                            act("$n utters the words 'judicandus dies'.",
                                1, ch, 0, 0, TO_ROOM);
                            cast_cure_light(GetMaxLevel(ch), ch, "",
                                            SPELL_TYPE_SPELL, most_hurt,
                                            0);
                            return (TRUE);
                        }
                    }
                } else {        /* Other Team? */
                    if (IS_AFFECTED(most_hurt, AFF_TEAM_GREY)) {

                        if (check_nomagic(ch, 0, 0))
                            return (TRUE);

                        act("$n utters the words 'die punk'.",
                            1, ch, 0, 0, TO_ROOM);
                        cast_cause_light(GetMaxLevel(ch), ch, "",
                                         SPELL_TYPE_SPELL, most_hurt, 0);
                        return (TRUE);
                    } else
                        return (FALSE);
                }
            } else {            /* I'm asleep or sitting */
                return (FALSE);
            }
        }
    }
    return (FALSE);
}
#endif

int blink(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
          int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (GET_HIT(ch) < GET_MAX_HIT(ch) / 3) {
        act("$n blinks.", TRUE, ch, 0, 0, TO_ROOM);
        cast_teleport(12, ch, "", SPELL_TYPE_SPELL, ch, 0);
        return (TRUE);
    } else {
        return (FALSE);
    }
}

int MidgaardCitizen(struct char_data *ch, int cmd, char *arg,
                    struct char_data *mob, int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        fighter(ch, cmd, arg, mob, type);

        if (check_soundproof(ch)) {
            return (FALSE);
        }
        if (number(0, 18) == 0) {
            do_shout(ch, "Guards! Help me! Please!", 0);
        } else {
            act("$n shouts 'Guards!  Help me! Please!'", TRUE, ch, 0, 0,
                TO_ROOM);
        }

        if (ch->specials.fighting) {
            CallForGuard(ch, ch->specials.fighting, 3, MIDGAARD);
        }
        return (TRUE);
    }

    return (FALSE);
}

int ghoul(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
          int type)
{
    struct char_data *tar;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    tar = ch->specials.fighting;

    if (tar && tar->in_room == ch->in_room &&
        !IS_AFFECTED(tar, AFF_PROTECT_FROM_EVIL) &&
        !IS_AFFECTED(tar, AFF_SANCTUARY) &&
        HitOrMiss(ch, tar, CalcThaco(ch))) {

        act("$n touches $N!", 1, ch, 0, tar, TO_NOTVICT);
        act("$n touches you!", 1, ch, 0, tar, TO_VICT);
        if (!IS_AFFECTED(tar, AFF_PARALYSIS)) {
            cast_paralyze(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, tar, 0);
            return TRUE;
        }
    }
    return FALSE;
}

int CarrionCrawler(struct char_data *ch, int cmd, char *arg,
                   struct char_data *mob, int type)
{
    struct char_data *tar;
    int             i;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    for (i = 0; i < 8; i++) {
        if ((tar = FindAHatee(ch)) == NULL) {
            tar = FindVictim(ch);
        }
        if (tar && tar->in_room == ch->in_room &&
            HitOrMiss(ch, tar, CalcThaco(ch))) {
            act("$n touches $N!", 1, ch, 0, tar, TO_NOTVICT);
            act("$n touches you!", 1, ch, 0, tar, TO_VICT);
            if (!IS_AFFECTED(tar, AFF_PARALYSIS)) {
                cast_paralyze(GetMaxLevel(ch), ch, "",
                              SPELL_TYPE_SPELL, tar, 0);
                return TRUE;
            }
        }
    }
    return FALSE;
}

int WizardGuard(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    struct char_data *tch,
                   *evil;
    int             max_evil;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        fighter(ch, cmd, arg, mob, type);
        CallForGuard(ch, ch->specials.fighting, 9, MIDGAARD);
    }
    max_evil = 1000;
    evil = 0;

    for (tch = real_roomp(ch->in_room)->people; tch;
         tch = tch->next_in_room) {
        if (tch->specials.fighting && GET_ALIGNMENT(tch) < max_evil &&
            (IS_NPC(tch) || IS_NPC(tch->specials.fighting))) {
            max_evil = GET_ALIGNMENT(tch);
            evil = tch;
        }
    }

    if (evil && (GET_ALIGNMENT(evil->specials.fighting) >= 0) &&
        !check_peaceful(ch, "")) {
        if (!check_soundproof(ch)) {
            act("$n screams 'DEATH!!!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
        }
        hit(ch, evil, TYPE_UNDEFINED);
        return (TRUE);
    }
    return (FALSE);
}

int vampire(struct char_data *ch, int cmd, char *arg,
            struct char_data *mob, int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting &&
        ch->specials.fighting->in_room == ch->in_room) {
        act("$n touches $N!", 1, ch, 0, ch->specials.fighting, TO_NOTVICT);
        act("$n touches you!", 1, ch, 0, ch->specials.fighting, TO_VICT);
        cast_energy_drain(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                          ch->specials.fighting, 0);

        if (ch->specials.fighting &&
            ch->specials.fighting->in_room == ch->in_room) {
            cast_energy_drain(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                              ch->specials.fighting, 0);
        }
        return TRUE;
    }
    return FALSE;
}

int wraith(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
           int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting &&
        ch->specials.fighting->in_room == ch->in_room) {
        act("$n touches $N!", 1, ch, 0, ch->specials.fighting, TO_NOTVICT);
        act("$n touches you!", 1, ch, 0, ch->specials.fighting, TO_VICT);
        cast_energy_drain(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                          ch->specials.fighting, 0);
        return TRUE;
    }
    return FALSE;
}

int shadow(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
           int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting &&
        ch->specials.fighting->in_room == ch->in_room) {
        act("$n touches $N!", 1, ch, 0, ch->specials.fighting, TO_NOTVICT);
        act("$n touches you!", 1, ch, 0, ch->specials.fighting, TO_VICT);
        cast_chill_touch(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                         ch->specials.fighting, 0);
        if (ch->specials.fighting) {
            cast_weakness(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                          ch->specials.fighting, 0);
        }
        return TRUE;
    }
    return FALSE;
}

int geyser(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
           int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (number(0, 3) == 0) {
        act("You erupt.", 1, ch, 0, 0, TO_CHAR);
        cast_geyser(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, 0, 0);
        send_to_zone("The volcano rumbles slightly.\n\r", ch);
        return (TRUE);
    }
    return( FALSE );
}

int green_slime(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    struct char_data *cons;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    for (cons = real_roomp(ch->in_room)->people; cons;
         cons = cons->next_in_room) {
        if (!IS_NPC(cons) && GetMaxLevel(cons) < LOW_IMMORTAL) {
            cast_green_slime(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                             cons, 0);
        }
    }
    return( TRUE );
}

int DracoLich(struct char_data *ch, int cmd, char *arg,
              struct char_data *mob, int type)
{
    return( FALSE );
}

int Drow(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
         int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (!ch->specials.fighting) {
        if (!affected_by_spell(ch, SPELL_GLOBE_DARKNESS)
            && !IS_AFFECTED(ch, AFF_DARKNESS)) {
            act("$n uses $s innate powers of darkness", FALSE, ch, 0, 0,
                TO_ROOM);
            cast_globe_darkness(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                                ch, 0);
            return (TRUE);
        }

        if (ch->specials.hunting) {
            act("$n uses $s innate powers of levitation", FALSE, ch, 0, 0,
                TO_ROOM);
            cast_flying(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, ch, 0);
            return (TRUE);
        }
    } else {
        if (!affected_by_spell(ch->specials.fighting, SPELL_BLINDNESS) &&
            number(1, 100) > 25) {
            /*
             * simulate casting darkness on a person...
             */
            act("$n uses $s innate powers of darkness on $N!", FALSE, ch,
                0, ch->specials.fighting, TO_NOTVICT);
            act("$n drops a pitch black globe around you!", FALSE, ch, 0,
                ch->specials.fighting, TO_VICT);
            cast_blindness(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                           ch->specials.fighting, 0);
            return (TRUE);
        }

        if (!affected_by_spell(ch->specials.fighting, SPELL_FAERIE_FIRE)
            && number(1, 100) > 50) {
            /*
             * simulate faerie fire
             */
            act("$n tries to outline $N with $s faerie fire!", FALSE, ch,
                0, ch->specials.fighting, TO_NOTVICT);
            act("$n tries to outline you with a faerie fire glow!", FALSE,
                ch, 0, ch->specials.fighting, TO_VICT);
            cast_faerie_fire(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                             ch->specials.fighting, 0);
            return (TRUE);
        }
    }

    return (archer(ch, cmd, arg, mob, type));
}

int thief(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
          int type)
{
    struct char_data *cons;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (GET_POS(ch) != POSITION_STANDING) {
        return (FALSE);
    }
    for (cons = real_roomp(ch->in_room)->people; cons;
         cons = cons->next_in_room) {
        if (!IS_NPC(cons) && GetMaxLevel(cons) < LOW_IMMORTAL &&
            !number(0, 4)) {
            npc_steal(ch, cons);
        }
    }

    return (FALSE);
}

/*
 ********************************************************************
 *  Special procedures for mobiles                                      *
 ******************************************************************** */

int guild_guard(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    if (!cmd) {
        if (ch->specials.fighting) {
            return (fighter(ch, cmd, arg, mob, type));
        }
    } else {
        switch (ch->in_room) {
        case 3017:
            return (CheckForBlockedMove(ch, cmd, arg, 3017, 2,
                                        CLASS_MAGIC_USER | CLASS_SORCERER));
            break;
        case 3004:
            return (CheckForBlockedMove(ch, cmd, arg, 3004, 0, CLASS_CLERIC));
            break;
        case 3027:
            return (CheckForBlockedMove(ch, cmd, arg, 3027, 1, CLASS_THIEF));
            break;
        case 3021:
            return (CheckForBlockedMove(ch, cmd, arg, 3021, 1, CLASS_WARRIOR));
            break;
        }
    }

    return FALSE;
}

int Inquisitor(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        return (fighter(ch, cmd, arg, mob, type));
    }

    switch (ch->generic) {
    case INQ_SHOUT:
        if (!check_soundproof(ch))
            do_shout(ch, "NOONE expects the Spanish Inquisition!", 0);
        ch->generic = 0;
        break;
    default:
        break;
    }
    return (FALSE);
}

int puff(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
         int type)
{
    struct char_data *i,
                   *tmp_ch;
    char            buf[80];

    if (type == EVENT_DWARVES_STRIKE) {
        do_shout(ch, "Ack! Of all the stupid things! Those damned dwarves are"
                     " on strike again!", 0);
        return (TRUE);
    }

    if (type == EVENT_END_STRIKE) {
        do_shout(ch, "Gee, about time those dwarves stopped striking!", 0);
        return (TRUE);
    }

    if (type == EVENT_DEATH) {
        do_shout(ch, "Ack! I've been killed! Have some God Load me again!!!",
                 0);
        return (TRUE);
    }

    if (type == EVENT_SPRING) {
        do_shout(ch, "Ahhh, spring is in the air.", 0);
        return (TRUE);
    }

    if (cmd) {
        return (FALSE);
    }
    if (ch->generic == 1) {
        do_shout(ch, "When will we get there?", 0);
        ch->generic = 0;
    }

    if (check_soundproof(ch)) {
        return (FALSE);
    }
    switch (number(0, 250)) {
    case 0:
        sprintf(buf, "Anyone know where I am at?\n");
        do_say(ch, buf, 0);
        return (TRUE);
    case 1:
        do_say(ch, "How'd all those fish get up here?", 0);
        return (TRUE);
    case 2:
        do_say(ch, "I'm a very female dragon.", 0);
        return (TRUE);
    case 3:
        do_say(ch, "Haven't I seen you at the Temple?", 0);
        return (TRUE);
    case 4:
        do_shout(ch, "Bring out your dead, bring out your dead!", 0);
        return (TRUE);
    case 5:
        do_emote(ch, "gropes you.", 0);
        return (TRUE);
    case 6:
        do_emote(ch, "gives you a long and passionate kiss.  It seems to last"
                     " forever.", 0);
        return (TRUE);
    case 7:
        for (i = character_list; i; i = i->next) {
            if (!IS_NPC(i) && !number(0, 5)) {
                if (!strcmp(GET_NAME(i), "Celestian")) {
                    do_shout(ch, "Celestian, come ravish me now!", 0);
                } else if (!strcmp(GET_NAME(i), "Fiona")) {
                    do_shout(ch, "I'm Puff the PMS dragon!", 0);
                } else if (!strcmp(GET_NAME(i), "Stranger")) {
                    do_shout(ch, "People are strange, when they're with "
                                 "Stranger!", 0);
                } else if (!strcmp(GET_NAME(i), "God")) {
                    do_shout(ch, "God!  Theres only room for one smartass "
                                 "robot on this mud!", 0);
                } else if (GET_SEX(i) == SEX_MALE) {
                    sprintf(buf, "Hey, %s, how about some MUDSex?",
                            GET_NAME(i));
                    do_say(ch, buf, 0);
                } else {
                    sprintf(buf, "I'm much prettier than %s, don't you think?",
                            GET_NAME(i));
                    do_say(ch, buf, 0);
                }
            }
            break;
        }
        return (TRUE);
    case 8:
        do_say(ch, "Celestian is my hero!", 0);
        return (TRUE);
    case 9:
        do_say(ch, "So, wanna neck?", 0);
        return (TRUE);
    case 10:
        tmp_ch = (struct char_data *) FindAnyVictim(ch);
        if (IS_NPC(ch)) {
            return (FALSE);
        }

        sprintf(buf, "Party on, %s", GET_NAME(tmp_ch));
        do_say(ch, buf, 0);
        return (TRUE);
    case 11:
        if (!number(0, 30)) {
            do_shout(ch, "NOT!!!", 0);
        }
        return (TRUE);
    case 12:
        do_say(ch, "Bad news.  Termites.", 0);
        return (TRUE);
    case 13:
        for (i = character_list; i; i = i->next) {
            if (!IS_NPC(i) && !number(0, 30)) {
                sprintf(buf, "%s shout I love Celestian!", GET_NAME(i));
                do_force(ch, buf, 0);

                sprintf(buf, "%s bounce", GET_NAME(i));
                do_force(ch, buf, 0);

                do_restore(ch, GET_NAME(i), 0);
                return (TRUE);
            }
        }
        return (TRUE);
    case 14:
        do_say(ch, "I'll be back.", 0);
        return (TRUE);
    case 15:
        do_say(ch, "Aren't wombat's so cute?", 0);
        return (TRUE);
    case 16:
        do_emote(ch, "fondly fondles you.", 0);
        return (TRUE);
    case 17:
        do_emote(ch, "winks at you.", 0);
        return (TRUE);
    case 18:
        do_say(ch, "This mud is too silly!", 0);
        return (TRUE);
    case 19:
        do_say(ch, "If the Mayor is in a room alone, ", 0);
        do_say(ch, "Does he say 'Good morning citizens.'?", 0);
        return (TRUE);
    case 20:
        for (i = character_list; i; i = i->next) {
            if (!IS_NPC(i) && !number(0, 30)) {
                sprintf(buf, "Top of the morning to you %s!", GET_NAME(i));
                do_shout(ch, buf, 0);
                return (TRUE);
            }
        }
        break;
    case 21:
        for (i = real_roomp(ch->in_room)->people; i; i = i->next_in_room) {
            if (!IS_NPC(i) && !number(0, 3)) {
                sprintf(buf, "Pardon me, %s, but are those bugle boy jeans "
                             "you are wearing?", GET_NAME(i));
                do_say(ch, buf, 0);
                return (TRUE);
            }
        }
        break;
    case 22:
        for (i = real_roomp(ch->in_room)->people; i; i = i->next_in_room) {
            if (!IS_NPC(i) && !number(0, 3)) {
                sprintf(buf, "Pardon me, %s, but do you have any Grey Poupon?",
                        GET_NAME(i));
                do_say(ch, buf, 0);
                return (TRUE);
            }
        }
        break;
    case 23:
        if (number(0, 80) == 0) {
            do_shout(ch, "Where are we going?", 0);
            ch->generic = 1;
        }
        break;
    case 24:
        do_say(ch, "Blackstaff is a wimp!", 0);
        return (TRUE);
        break;
    case 25:
        do_say(ch, "Better be nice or I will user spellfire on you!", 0);
        return (TRUE);
        break;
    case 26:
        if (number(0, 100) == 0) {
            do_shout(ch, "What is the greatest joy?", 0);
        }
        break;
    case 27:
        do_say(ch, "Have you see Elminster? Gads he is slow...", 0);
        return (TRUE);
    case 28:
        if (number(0, 50)) {
            do_shout(ch, "SAVE!  I'm running out of cute things to say!", 0);
        }
        do_force(ch, "all save", 0);
        return (TRUE);
    case 29:
        do_say(ch, "I hear Strahd is a really mean vampire.", 0);
        return (TRUE);
    case 30:
        do_say(ch, "I heard there was a sword that would kill frost giants.",
               0);
        return (TRUE);
    case 31:
        do_say(ch, "Hear about the sword that kills Red Dragons?", 0);
        return (TRUE);
    case 32:
        if (number(0, 100) == 0) {
            do_shout(ch, "Help yourself and help a newbie!", 0);
            return (TRUE);
        }
        break;
    case 33:
        if (number(0, 100) == 0) {
             do_shout(ch, "Kill all other dragons!", 0);
            return (TRUE);
        }
        break;
    case 34:
        if (number(0, 50) == 0) {
            for (i = character_list; i; i = i->next) {
                if (mob_index[i->nr].func == Inquisitor) {
                    do_shout(ch, "I wasn't expecting the Spanish Inquisition!",
                             0);
                    i->generic = INQ_SHOUT;
                    return (TRUE);
                }
            }
            return (TRUE);
        }
        break;
    case 35:
        do_say(ch, "Are you crazy, is that your problem?", 0);
        return (TRUE);
    case 36:
        for (i = real_roomp(ch->in_room)->people; i; i = i->next_in_room) {
            if (!IS_NPC(i) && !number(0, 3)) {
                sprintf(buf, "%s, do you think I'm going bald?", GET_NAME(i));
                do_say(ch, buf, 0);
                return (TRUE);
            }
        }
        break;
    case 37:
        do_say(ch, "This is your brain.", 0);
        do_say(ch, "This is MUD.", 0);
        do_say(ch, "This is your brain on MUD.", 0);
        do_say(ch, "Any questions?", 0);
        return (TRUE);
    case 38:
        for (i = character_list; i; i = i->next) {
            if (!IS_NPC(i) && !number(0, 20) && i->in_room != NOWHERE) {
                sprintf(buf, "%s save", GET_NAME(i));
                do_force(ch, buf, 0);
            }
        }
        return (TRUE);
    case 39:
        do_say(ch, "I'm Puff the Magic Dragon, who the hell are you?", 0);
        return (TRUE);
    case 40:
        do_say(ch, "Attention all planets of the Solar Federation!", 0);
        do_say(ch, "We have assumed control.", 0);
        return (TRUE);
    case 41:
        if (!number(0, 50)) {
            do_shout(ch, "We need more explorers!", 0);
            return (TRUE);
        }
        break;
    case 42:
        if (!number(0, 50)) {
            do_shout(ch, "Pray to Celestian, he might be in a good mood!", 0);
            return (TRUE);
        }
        break;
    case 43:
        do_say(ch, "Pardon me boys, is this the road to Great Cthulhu?", 0);
        return (TRUE);
    case 44:
        do_say(ch, "May the Force be with you... Always.", 0);
        return (TRUE);
    case 45:
        do_say(ch, "Eddies in the space time continuum.", 0);
        return (TRUE);
    case 46:
        do_say(ch, "Quick!  Reverse the polarity of the neutron flow!", 0);
        return (TRUE);
    case 47:
        if (!number(0, 50)) {
            do_shout(ch, "Someone pray to Blackmouth, he is lonely.", 0);
            return (TRUE);
        }
        break;
    case 48:
        do_say(ch, "Shh...  I'm beta testing.  I need complete silence!", 0);
        return (TRUE);
    case 49:
        do_say(ch, "Do you have any more of that Plutonium Nyborg!", 0);
        return (TRUE);
    case 50:
        do_say(ch, "I'm the real implementor, you know.", 0);
        return (TRUE);
    case 51:
        do_emote(ch, "moshes into you almost causing you to fall.", 0);
        return (TRUE);
    case 52:
        if (!number(0, 30)) {
            do_shout(ch, "Everybody pray to Ator!", 0);
        }
        return (TRUE);
    case 53:
        do_say(ch, "You know I always liked you the best don't you?", 0);
        do_emote(ch, "winks seductively at you.", 0);
        return (TRUE);
    case 54:
        if (!number(0, 30)) {
            do_shout(ch, "Ack! Who prayed to Wert!", 0);
        }
        return (TRUE);

    default:
        return (0);
    }
    return( FALSE );
}

int regenerator(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    if (cmd) {
        return (FALSE);
    }
    if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
        GET_HIT(ch) += 9;
        GET_HIT(ch) = MIN(GET_HIT(ch), GET_MAX_HIT(ch));

        act("$n regenerates.", TRUE, ch, 0, 0, TO_ROOM);
        return (TRUE);
    }
    return( FALSE );
}

int replicant(struct char_data *ch, int cmd, char *arg,
              struct char_data *mob1, int type)
{
    struct char_data *mob;

    if (cmd) {
        return FALSE;
    }
    if (GET_HIT(ch) < GET_MAX_HIT(ch)) {
        act("Drops of $n's blood hit the ground, and spring up into another "
            "one!", TRUE, ch, 0, 0, TO_ROOM);
        mob = read_mobile(ch->nr, REAL);
        char_to_room(mob, ch->in_room);
        act("Two undamaged opponents face you now.", TRUE, ch, 0, 0, TO_ROOM);
        GET_HIT(ch) = GET_MAX_HIT(ch);
    }

    return FALSE;
}

#define TYT_NONE 0
#define TYT_CIT  1
#define TYT_WHAT 2
#define TYT_TELL 3
#define TYT_HIT  4

int Tytan(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
          int type)
{
    struct char_data *vict;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        return (magic_user(ch, cmd, arg, mob, type));
    }

    switch (ch->generic) {
    case TYT_NONE:
        if ((vict = FindVictim(ch))) {
            ch->generic = TYT_CIT;
            SetHunting(ch, vict);
        }
        break;
    case TYT_CIT:
        if (ch->specials.hunting) {
            if (IS_SET(ch->specials.act, ACT_AGGRESSIVE)) {
                REMOVE_BIT(ch->specials.act, ACT_AGGRESSIVE);
            }
            if (ch->in_room == ch->specials.hunting->in_room) {
                act("Where is the Citadel?", TRUE, ch, 0, 0, TO_ROOM);
                ch->generic = TYT_WHAT;
            }
        } else {
            ch->generic = TYT_NONE;
        }
        break;
    case TYT_WHAT:
        if (ch->specials.hunting) {
            if (ch->in_room == ch->specials.hunting->in_room) {
                act("What must we do?", TRUE, ch, 0, 0, TO_ROOM);
                ch->generic = TYT_TELL;
            }
        } else {
            ch->generic = TYT_NONE;
        }
        break;
    case TYT_TELL:
        if (ch->specials.hunting) {
            if (ch->in_room == ch->specials.hunting->in_room) {
                act("Tell Us!  Command Us!", TRUE, ch, 0, 0, TO_ROOM);
                ch->generic = TYT_HIT;
            }
        } else {
            ch->generic = TYT_NONE;
        }
        break;
    case TYT_HIT:
        if (ch->specials.hunting) {
            if (ch->in_room == ch->specials.hunting->in_room) {
                if (!check_peaceful(ch, "The Tytan screams in anger")) {
                    hit(ch, ch->specials.hunting, TYPE_UNDEFINED);
                    if (!IS_SET(ch->specials.act, ACT_AGGRESSIVE)) {
                        SET_BIT(ch->specials.act, ACT_AGGRESSIVE);
                    }
                    ch->generic = TYT_NONE;
                } else {
                    ch->generic = TYT_CIT;
                }
            }
        } else {
            ch->generic = TYT_NONE;
        }
        break;
    default:
        ch->generic = TYT_NONE;
    }
    return (FALSE);
}

int AbbarachDragon(struct char_data *ch, int cmd, char *arg,
                   struct char_data *mob, int type)
{
    struct char_data *targ;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (!ch->specials.fighting) {
        targ = (struct char_data *) FindAnyVictim(ch);
        if (targ && !check_peaceful(ch, "")) {
            hit(ch, targ, TYPE_UNDEFINED);
            act("You have now payed the price of crossing.",
                TRUE, ch, 0, 0, TO_ROOM);
            return (TRUE);
        }
    } else {
        return (BreathWeapon(ch, cmd, arg, mob, type));
    }
    return( FALSE );
}

int fido(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
         int type)
{
    register struct obj_data *i,
                   *temp,
                   *next_obj,
                   *next_r_obj;
    register struct char_data *v,
                   *next;
    register struct room_data *rp;
    char            found = FALSE;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if ((rp = real_roomp(ch->in_room)) == 0) {
        return (FALSE);
    }
    for (v = rp->people; (v && (!found)); v = next) {
        next = v->next_in_room;
        if (IS_NPC(v) && mob_index[v->nr].virtual == 100 && CAN_SEE(ch, v)) {
            /*
             * is a zombie
             */
            if (v->specials.fighting) {
                stop_fighting(v);
            }
            make_corpse(v, '\0');
            extract_char(v);
            found = TRUE;
        }
    }

    for (i = real_roomp(ch->in_room)->contents; i; i = next_r_obj) {
        next_r_obj = i->next_content;
        if (GET_ITEM_TYPE(i) == ITEM_CONTAINER && i->obj_flags.value[3]) {
            act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
            for (temp = i->contains; temp; temp = next_obj) {
                next_obj = temp->next_content;
                obj_from_obj(temp);
                obj_to_room(temp, ch->in_room);
            }
            extract_obj(i);
            return (TRUE);
        }
    }
    return (FALSE);
}

int janitor(struct char_data *ch, int cmd, char *arg,
            struct char_data *mob, int type)
{
    struct obj_data *i;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    for (i = real_roomp(ch->in_room)->contents; i; i = i->next_content) {
        if (IS_SET(i->obj_flags.wear_flags, ITEM_TAKE) &&
            (i->obj_flags.type_flag == ITEM_DRINKCON ||
             i->obj_flags.cost <= 10)) {
            act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
            obj_from_room(i);
            obj_to_char(i, ch);
            return (TRUE);
        }
    }
    return (FALSE);
}

int tormentor(struct char_data *ch, int cmd, char *arg,
              struct char_data *mob, int type)
{
    if (!cmd && IS_IMMORTAL(ch) && !IS_PC(ch)) {
        return (FALSE);
    }

    return (TRUE);
}

int RustMonster(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    struct char_data *vict;
    struct obj_data *t_item;
    int             t_pos;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    /*
     * find a victim
     */
    if (ch->specials.fighting) {
        vict = ch->specials.fighting;
    } else {
        vict = FindVictim(ch);
        if (!vict) {
            return (FALSE);
        }
    }

    /*
     * choose an item of armor or a weapon that is metal
     * since metal isn't defined, we'll just use armor and weapons
     */

    /*
     * choose a weapon first, then if no weapon, choose a shield,
     * if no shield, choose breast plate, then leg plate, sleeves,
     * helm
     */

    if (vict->equipment[WIELD]) {
        t_item = vict->equipment[WIELD];
        t_pos = WIELD;
    } else if (vict->equipment[WEAR_SHIELD]) {
        t_item = vict->equipment[WEAR_SHIELD];
        t_pos = WEAR_SHIELD;
    } else if (vict->equipment[WEAR_BODY]) {
        t_item = vict->equipment[WEAR_BODY];
        t_pos = WEAR_BODY;
    } else if (vict->equipment[WEAR_LEGS]) {
        t_item = vict->equipment[WEAR_LEGS];
        t_pos = WEAR_LEGS;
    } else if (vict->equipment[WEAR_ARMS]) {
        t_item = vict->equipment[WEAR_ARMS];
        t_pos = WEAR_ARMS;
    } else if (vict->equipment[WEAR_HEAD]) {
        t_item = vict->equipment[WEAR_HEAD];
        t_pos = WEAR_HEAD;
    } else {
        return (FALSE);
    }

    /*
     * item makes save (or not)
     */
    if (DamageOneItem(vict, ACID_DAMAGE, t_item)) {
        t_item = unequip_char(vict, t_pos);
        if (t_item) {
            /*
             * if it doesn't make save, falls into a pile of scraps
             */
            MakeScrap(vict, NULL, t_item);
        }
    }
    return (FALSE);
}

int temple_labrynth_liar(struct char_data *ch, int cmd, char *arg,
                         struct char_data *mob, int type)
{
    void            do_say(struct char_data *ch, char *argument, int cmd);

    if (cmd || !AWAKE(ch)) {
        return (0);
    }
    if (check_soundproof(ch)) {
        return (FALSE);
    }
    switch (number(0, 15)) {
    case 0:
        do_say(ch, "I'd go west if I were you.", 0);
        return (TRUE);
    case 1:
        do_say(ch, "I heard that Vile is a cute babe.", 0);
        return (TRUE);
    case 2:
        do_say(ch, "Going east will avoid the beast!", 0);
        return (TRUE);
    case 4:
        do_say(ch, "North is the way to go.", 0);
        return (TRUE);
    case 6:
        do_say(ch, "Dont dilly dally go south.", 0);
        return (TRUE);
    case 8:
        do_say(ch, "Great treasure lies ahead", 0);
        return (TRUE);
    case 10:
        do_say(ch, "I wouldn't kill the sentry if I were more than level 9. "
                   "No way!", 0);
        return (TRUE);
    case 12:
        do_say(ch, "I am a very clever liar.", 0);
        return (TRUE);
    case 14:
        do_say(ch, "Loki is a really great guy!", 0);
        do_say(ch, "Well.... maybe not...", 0);
        return (TRUE);
    default:
        do_say(ch, "Then again I could be wrong!", 0);
        return (TRUE);
    }
}

int temple_labrynth_sentry(struct char_data *ch, int cmd, char *arg,
                           struct char_data *mob, int type)
{
    struct char_data *tch;
    int             counter;

    if (cmd || !AWAKE(ch) || GET_POS(ch) != POSITION_FIGHTING ||
        !ch->specials.fighting || check_soundproof(ch)) {
        return (FALSE);
    }

    /*
     * Find a dude to do very evil things upon !
     */

    for (tch = real_roomp(ch->in_room)->people; tch;
         tch = tch->next_in_room) {
        if (GetMaxLevel(tch) > 10 && CAN_SEE(ch, tch)) {
            act("The sentry snaps out of his trance and ...", 1, ch, 0, 0,
                TO_ROOM);
            do_say(ch, "You will die for your insolence, pig-dog!", 0);
            for (counter = 0; counter < 4; counter++) {
                if (check_nomagic(ch, 0, 0)) {
                    return (FALSE);
                }
            }
            if (GET_POS(tch) > POSITION_SITTING) {
                cast_fireball(15, ch, "", SPELL_TYPE_SPELL, tch, 0);
            } else {
                return TRUE;
            }
            return TRUE;
        } else {
            act("The sentry looks concerned and continues to push you away",
                1, ch, 0, 0, TO_ROOM);
            do_say(ch, "Leave me alone. My vows do not permit me to kill you!",
                   0);
        }
    }
    return TRUE;
}

#if 0
#define WW_LOOSE 0
#define WW_FOLLOW 1

int Whirlwind(struct char_data *ch, int cmd, char *arg,
              struct char_data *mob, int type)
{
    struct char_data *tmp;
    const char     *names[] = { "Loki", "Belgarath", 0 };
    int             i = 0;

    if (ch->in_room == -1)
        return (FALSE);

    if (cmd == 0 && ch->generic == WW_LOOSE) {
        for (tmp = real_roomp(ch->in_room)->people; tmp;
             tmp = tmp->next_in_room) {
            while (names[i]) {
                if (!strcmp(GET_NAME(tmp), names[i])
                    && ch->generic == WW_LOOSE) {
                    /*
                     * start following
                     */
                    if (circle_follow(ch, tmp))
                        return (FALSE);
                    if (ch->master)
                        stop_follower(ch);
                    add_follower(ch, tmp);
                    ch->generic = WW_FOLLOW;
                }
                i++;
            }
        }
        if (ch->generic == WW_LOOSE && !cmd) {
            act("The $n suddenly dissispates into nothingness.", 0, ch, 0,
                0, TO_ROOM);
            extract_char(ch);
        }
    }
}
#endif

#define NN_LOOSE  0
#define NN_FOLLOW 1
#define NN_STOP   2

int NudgeNudge(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{

    struct char_data *vict;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        return (FALSE);
    }

    switch (ch->generic) {
    case NN_LOOSE:
        /*
         * find a victim
         */
        vict = FindVictim(ch);
        if (!vict) {
            return (FALSE);
        }
        /*
         * start following
         */
        if (circle_follow(ch, vict)) {
            return (FALSE);
        }
        if (ch->master) {
            stop_follower(ch);
        }
        add_follower(ch, vict);
        ch->generic = NN_FOLLOW;
        if (!check_soundproof(ch)) {
            do_say(ch, "Good Evenin' Squire!", 0);
        }
        act("$n nudges you.", FALSE, ch, 0, 0, TO_CHAR);
        act("$n nudges $N.", FALSE, ch, 0, ch->master, TO_ROOM);
        break;
    case NN_FOLLOW:
        switch (number(0, 20)) {
        case 0:
            if (!check_soundproof(ch)) {
                do_say(ch, "Is your wife a goer?  Know what I mean, eh?", 0);
            }
            act("$n nudges you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n nudges $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            break;
        case 1:
            act("$n winks at you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n winks at you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n winks at $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            act("$n winks at $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            act("$n nudges you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n nudges $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            act("$n nudges you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n nudges $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            if (!check_soundproof(ch)) {
                do_say(ch, "Say no more!  Say no MORE!", 0);
            }
            break;
        case 2:
            if (!check_soundproof(ch)) {
                do_say(ch, "You been around, eh?", 0);
                do_say(ch, "...I mean you've ..... done it, eh?", 0);
            }
            act("$n nudges you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n nudges $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            act("$n nudges you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n nudges $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            break;
        case 3:
            if (!check_soundproof(ch)) {
                do_say(ch, "A nod's as good as a wink to a blind bat, eh?", 0);
            }
            act("$n nudges you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n nudges $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            act("$n nudges you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n nudges $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            break;
        case 4:
            if (!check_soundproof(ch)) {
                do_say(ch, "You're WICKED, eh!  WICKED!", 0);
            }
            act("$n winks at you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n winks at you.", FALSE, ch, 0, 0, TO_CHAR);
            act("$n winks at $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            act("$n winks at $N.", FALSE, ch, 0, ch->master, TO_ROOM);
            break;
        case 5:
            if (!check_soundproof(ch)) {
                do_say(ch, "Wink. Wink.", 0);
            }
            break;
        case 6:
            if (!check_soundproof(ch)) {
                do_say(ch, "Nudge. Nudge.", 0);
            }
            break;
        case 7:
        case 8:
            ch->generic = NN_STOP;
            break;
        default:
            break;
        }
        break;
    case NN_STOP:
        /*
         * Stop following
         */
        if (!check_soundproof(ch)) {
            do_say(ch, "Evening, Squire", 0);
        }
        stop_follower(ch);
        ch->generic = NN_LOOSE;
        break;
    default:
        ch->generic = NN_LOOSE;
        break;
    }
    return (TRUE);
}

int AGGRESSIVE(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{
    struct char_data *i,
                   *next;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (check_peaceful(ch, "")) {
        return FALSE;
    }
    if (ch->in_room > -1) {
        for (i = real_roomp(ch->in_room)->people; i; i = next) {
            next = i->next_in_room;
            if (GET_RACE(i) != GET_RACE(ch) && !IS_IMMORTAL(i) &&
                CAN_SEE(ch, i)) {
                hit(ch, i, TYPE_UNDEFINED);
                break;
            }
        }
    }
    return( TRUE );
}

int citizen(struct char_data *ch, int cmd, char *arg,
            struct char_data *mob, int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        fighter(ch, cmd, arg, mob, type);

        if (check_soundproof(ch)) {
            return (FALSE);
        }
        if (!number(0, 18)) {
            do_shout(ch, "Guards! Help me! Please!", 0);
        } else {
            act("$n shouts 'Guards!  Help me! Please!'", TRUE, ch, 0, 0,
                TO_ROOM);
        }
    }
    return (FALSE);
}

int MidgaardCityguard(struct char_data *ch, int cmd, char *arg,
                      struct char_data *mob, int type)
{
    return (GenericCityguardHateUndead(ch, cmd, arg, mob, MIDGAARD));
}

#define ONE_RING 1105
int Ringwraith(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{
#if 0
    static char     buf[256];
    struct char_data *victim;
    static int      howmanyrings = -1;
    struct obj_data *ring;
    struct wraith_hunt {
        int             ringnumber;
        int             chances;
    }              *wh;
    int             rnum,
                    dir;
#endif

    if (!AWAKE(ch) || !IS_NPC(ch) || cmd) {
        return (FALSE);
    }

    if (ch->specials.fighting) {
        if (GET_POS(ch) > POSITION_SLEEPING &&
            GET_POS(ch) < POSITION_FIGHTING) {
            do_stand(ch, "", 0);
        } else {
            wraith(ch, cmd, arg, mob, type);
        }

        return (FALSE);
    }
    if (number(1, 25) == 1) {
        if (!check_soundproof(ch)) {
            act("$n says 'Give me The Ring *NOW*'.", FALSE, mob, NULL,
                NULL, TO_ROOM);
        } else {
            act("$n pokes you in the ribs very painfully.", FALSE, mob,
                NULL, NULL, TO_ROOM);
        }
        return (TRUE);
    } else {
        return (FALSE);
    }

#if 0
/*
 * Disable the one ring proc...
 */
    if (howmanyrings == -1) {   /* how many one rings are in the game? */
        howmanyrings = 1;
        get_obj_vis_world(ch, "999.one ring.", &howmanyrings);
    }

    if (ch->act_ptr == 0) {     /* does our ringwraith have his state
                                 * info? */
        ch->act_ptr = (struct wraith_hunt *) malloc(sizeof(*wh));
        /* this will never get freed  :( */
        wh = (struct wraith_hunt *) ch->act_ptr;
        wh->ringnumber = 0;
    } else {
        /* yuck, talk to loki about the act_ptr */
        wh = (struct wraith_hunt *) ch->act_ptr;
    }

    if (!wh->ringnumber) {
        /* is he currently tracking a ring */
        wh->chances = 0;
        wh->ringnumber = number(1, howmanyrings++);
    }

    /* where is this ring? */
    sprintf(buf, "%d.one ring.", (int) wh->ringnumber);

    if (!(ring = get_obj_vis_world(ch, buf, NULL))) {
        /*
         * there aren't as many one rings in the game as we thought
         */
        howmanyrings = 1;
        get_obj_vis_world(ch, "999.one ring.", &howmanyrings);
        wh->ringnumber = 0;
        return FALSE;
    }

    rnum = room_of_object(ring);

    if (rnum != ch->in_room) {
        dir = find_path(ch->in_room, is_target_room_p, (void *) rnum, -5000, 0);
        if (dir < 0) {
            /* we can't find the ring */
            wh->ringnumber = 0;
            return FALSE;
        }
        go_direction(ch, dir);
        return TRUE;
    }

    /*
     * the ring is in the same room!
     */

    if (victim = char_holding(ring)) {
        if (victim == ch) {
            obj_from_char(ring);
            extract_obj(ring);
            wh->ringnumber = 0;
            act("$n grimaces happily.", FALSE, ch, NULL, victim, TO_ROOM);
        } else {
            switch (wh->chances) {
            case 0:
                do_wake(ch, GET_NAME(victim), 0);
                if (!check_soundproof(ch))
                    act("$n says '$N, give me The Ring'.", FALSE, ch, NULL,
                        victim, TO_ROOM);
                else
                    act("$n pokes you in the ribs.", FALSE, ch, NULL,
                        victim, TO_ROOM);
                wh->chances++;
                return (TRUE);
                break;
            case 1:
                if (IS_NPC(victim)
                    && !IS_SET(victim->specials.act, ACT_POLYSELF)) {
#if 0
                    act("$N quickly surrenders The Ring to $n.", FALSE, ch,
                        NULL, victim, TO_ROOM);
                    if (ring->carried_by)
                        obj_from_char(ring);
                    else if (ring->equipped_by)
                        unequip_char(victim, ring->eq_pos);
                    obj_to_char(ring, ch);
#else
                    /*
                     * this otta suprize them! Purge a mob
                     */
                    for (ring = victim->carrying; ring;
                         extract_obj(ring), ring = ring->next_content);
                    do_purge(ch, GET_NAME(victim), 0);
#endif
                } else {
                    if (!check_soundproof(ch))
                        act("$n says '$N, give me The Ring *NOW*'.",
                            FALSE, ch, NULL, victim, TO_ROOM);
                    else {
                        act("$n pokes you in the ribs very painfully.",
                            FALSE, ch, NULL, victim, TO_ROOM);
                    }

                    wh->chances++;
                }
                return (TRUE);
                break;
            default:
                if (check_peaceful(ch, "Damn, he's in a safe spot.")) {
                    if (!check_soundproof(ch))
                        act("$n says 'You can't stay here forever, $N'.",
                            FALSE, ch, NULL, victim, TO_ROOM);
                } else {
                    if (!check_soundproof(ch))
                        act("$n says 'I guess I'll just have to get it "
                            "myself'.", FALSE, ch, NULL, victim, TO_ROOM);
                    hit(ch, victim, TYPE_UNDEFINED);
                }
                break;
            }
        }
    } else if (ring->in_obj) {
        /*
         * the ring is in an object
         */
        obj_from_obj(ring);
        obj_to_char(ring, ch);
        act("$n gets the One Ring.", FALSE, ch, NULL, victim, TO_ROOM);
    } else if (ring->in_room != NOWHERE) {
        obj_from_room(ring);
        obj_to_char(ring, ch);
        act("$n gets the Ring.", FALSE, ch, NULL, 0, TO_ROOM);
    } else {
        Log("a One Ring was completely disconnected!?");
        wh->ringnumber = 0;
    }
    return TRUE;

#endif
}

int WarrenGuard(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    struct char_data *tch,
                   *good;
    int             max_good;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        fighter(ch, cmd, arg, mob, type);
        return (TRUE);
    }

    max_good = -1000;
    good = 0;

    if (check_peaceful(ch, "")) {
        return FALSE;
    }
    for (tch = real_roomp(ch->in_room)->people; tch;
         tch = tch->next_in_room) {
        if (tch->specials.fighting && GET_ALIGNMENT(tch) > max_good &&
            (IS_NPC(tch) || IS_NPC(tch->specials.fighting))) {
            max_good = GET_ALIGNMENT(tch);
            good = tch;
        }
    }

    if (good && GET_ALIGNMENT(good->specials.fighting) <= 0) {
        if (!check_soundproof(ch)) {
            act("$n screams 'DEATH TO GOODY-GOODIES!!!!'",
                FALSE, ch, 0, 0, TO_ROOM);
        }
        hit(ch, good, TYPE_UNDEFINED);
        return (TRUE);
    }

    return (FALSE);
}

int zm_tired(struct char_data *zmaster)
{
    return (GET_HIT(zmaster) < GET_MAX_HIT(zmaster) / 2 ||
            GET_MANA(zmaster) < 40);
}

int zm_stunned_followers(struct char_data *zmaster)
{
    struct follow_type *fwr;

    for (fwr = zmaster->followers; fwr; fwr = fwr->next) {
        if (GET_POS(fwr->follower) == POSITION_STUNNED) {
            return TRUE;
        }
    }
    return FALSE;
}

void zm_init_combat(struct char_data * zmaster, struct char_data * target)
{
    struct follow_type *fwr;
    for (fwr = zmaster->followers; fwr; fwr = fwr->next) {
        if (IS_AFFECTED(fwr->follower, AFF_CHARM) &&
            fwr->follower->specials.fighting == NULL &&
            fwr->follower->in_room == target->in_room) {
            if (GET_POS(fwr->follower) == POSITION_STANDING) {
                hit(fwr->follower, target, TYPE_UNDEFINED);
            } else if (GET_POS(fwr->follower) > POSITION_SLEEPING &&
                       GET_POS(fwr->follower) < POSITION_FIGHTING) {
                do_stand(fwr->follower, "", -1);
            }
        }
    }
}

int zm_kill_fidos(struct char_data *zmaster)
{
    struct char_data *fido_b;

    fido_b = find_mobile_here_with_spec_proc(fido, zmaster->in_room);
    if (fido_b) {
        if (!check_soundproof(zmaster)) {
            act("$n shrilly screams 'Kill that carrion beast!'", FALSE,
                zmaster, 0, 0, TO_ROOM);
            zm_init_combat(zmaster, fido_b);
        }
        return TRUE;
    }
    return FALSE;
}

int zm_kill_aggressor(struct char_data *zmaster)
{
    struct follow_type *fwr;

    if (zmaster->specials.fighting && !check_soundproof(zmaster)) {
        act("$n bellows 'Kill that mortal that dares lay hands on me!'",
            FALSE, zmaster, 0, 0, TO_ROOM);
        zm_init_combat(zmaster, zmaster->specials.fighting);
        return TRUE;
    }

    for (fwr = zmaster->followers; fwr; fwr = fwr->next) {
        if (fwr->follower->specials.fighting &&
            IS_AFFECTED(fwr->follower, AFF_CHARM)) {
            if (!check_soundproof(zmaster)) {
                act("$n bellows 'Assist your brethren, my loyal servants!'",
                    FALSE, zmaster, 0, 0, TO_ROOM);
                zm_init_combat(zmaster, fwr->follower->specials.fighting);
                return TRUE;
            }
        }
    }
    return FALSE;
}


#define ZM_MANA 10
#define ZM_NEMESIS 3060

int zombie_master(struct char_data *ch, int cmd, char *arg,
                  struct char_data *mob, int type)
{
    struct obj_data *temp1;
    struct char_data *zmaster;
    int             dir;

    zmaster = find_mobile_here_with_spec_proc(zombie_master, ch->in_room);

    if (cmd != 0 || ch != zmaster || !AWAKE(ch)) {
        return FALSE;
    }
    if (!check_peaceful(ch, "") && (zm_kill_fidos(zmaster) ||
                                    zm_kill_aggressor(zmaster))) {
        do_stand(zmaster, "", -1);
        return TRUE;
    }

    switch (GET_POS(zmaster)) {
    case POSITION_RESTING:
        if (!zm_tired(zmaster)) {
            do_stand(zmaster, "", -1);
        }
        break;
    case POSITION_SITTING:
        if (!zm_stunned_followers(zmaster)) {
            if (!check_soundproof(ch)) {
                act("$n says 'It took you long enough...'", FALSE,
                    zmaster, 0, 0, TO_ROOM);
            }
            do_stand(zmaster, "", -1);
        }
        break;
    case POSITION_STANDING:
        if (zm_tired(zmaster)) {
            do_rest(zmaster, "", -1);
            return TRUE;
        }

        temp1 = get_obj_in_list_vis(zmaster, "corpse",
                                    real_roomp(zmaster->in_room)->contents);

        if (temp1) {
            if (GET_MANA(zmaster) < ZM_MANA) {
                if (dice(1, 20) == 1 && !check_soundproof(ch)) {
                    act("$n says 'So many bodies, so little time' and sighs.",
                        FALSE, zmaster, 0, 0, TO_ROOM);
                }
            } else {
                if (check_nomagic(ch, 0, 0)) {
                    return (FALSE);
                }
                if (!check_soundproof(ch)) {
                    act("$n says 'Wonderful, another loyal follower!' and "
                        "grins maniacly.", FALSE, zmaster, 0, 0, TO_ROOM);
                    GET_MANA(zmaster) -= ZM_MANA;
                    spell_animate_dead(GetMaxLevel(zmaster), ch, NULL, temp1);
                    if (zmaster->followers) {
                        /*
                         * assume the new follower is top of the list?
                         */
                        AddHatred(zmaster->followers->follower, OP_VNUM,
                                  ZM_NEMESIS);
                    }
                }
            }
            return TRUE;
        } else if (zm_stunned_followers(zmaster)) {
            do_sit(zmaster, "", -1);
            return TRUE;
        } else if (dice(1, 20) == 1) {
            act("$n searches for bodies.", FALSE, zmaster, 0, 0, TO_ROOM);
            return TRUE;
        } else if ((dir = find_path(zmaster->in_room, named_object_on_ground,
                                    "corpse", -200, 0)) >= 0) {
            go_direction(zmaster, dir);
            return TRUE;
        } else if (dice(1, 5) == 1) {
            act("$n can't find any bodies.", FALSE, zmaster, 0, 0, TO_ROOM);
            return TRUE;
        } else {
            mobile_wander(zmaster);
        }
    }

    return FALSE;
}

int pet_shops(struct char_data *ch, int cmd, char *arg,
              struct room_data *rp, int type)
{
    char            buf[MAX_STRING_LENGTH],
                    pet_name[256];
    int             pet_room;
    struct char_data *pet;

    pet_room = ch->in_room + 1;

    if (cmd == 59) {
        /*
         * List
         */
        send_to_char("Available pets are:\n\r", ch);
        for (pet = real_roomp(pet_room)->people; pet;
             pet = pet->next_in_room) {
            sprintf(buf, "%8d - %s\n\r", 24 * GET_EXP(pet),
                    pet->player.short_descr);
            send_to_char(buf, ch);
        }
        return (TRUE);
    }

    if (cmd == 56) {
        /*
         * Buy
         */
        arg = one_argument(arg, buf);
        only_argument(arg, pet_name);
        /*
         * Pet_Name is for later use when I feel like it
         */

        if (!(pet = get_char_room(buf, pet_room))) {
            send_to_char("There is no such pet!\n\r", ch);
            return (TRUE);
        }

        if (GET_GOLD(ch) < (GET_EXP(pet) * 10)) {
            send_to_char("You don't have enough gold!\n\r", ch);
            return (TRUE);
        }

        GET_GOLD(ch) -= GET_EXP(pet) * 10;

        pet = read_mobile(pet->nr, REAL);
        GET_EXP(pet) = 0;
        SET_BIT(pet->specials.affected_by, AFF_CHARM);

        if (*pet_name) {
            sprintf(buf, "%s %s", pet->player.name, pet_name);
            free(pet->player.name);
            pet->player.name = (char *) strdup(buf);

            sprintf(buf, "%sA small sign on a chain around the neck says 'My "
                         "Name is %s'\n\r", pet->player.description, pet_name);
            free(pet->player.description);
            pet->player.description = (char *) strdup(buf);
        }

        char_to_room(pet, ch->in_room);
        add_follower(pet, ch);

        IS_CARRYING_W(pet) = 0;
        IS_CARRYING_N(pet) = 0;

        send_to_char("May you enjoy your pet.\n\r", ch);
        act("$n bought $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

        return (TRUE);
    }

    /*
     * All commands except list and buy
     */
    return (FALSE);
}

int Fountain(struct char_data *ch, int cmd, char *arg,
             struct room_data *rp, int type)
{

    int             bits,
                    water;
    char            buf[MAX_INPUT_LENGTH];
    struct char_data *tmp_char;
    char            container[20];
    struct obj_data *obj;

    if (cmd == 248) {
        /*
         * fill
         */

        arg = one_argument(arg, buf);
        bits = generic_find(buf, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP,
                            ch, &tmp_char, &obj);

        if (!bits) {
            return (FALSE);
        }
        if (ITEM_TYPE(obj) != ITEM_DRINKCON) {
            send_to_char("Thats not a drink container!\n\r", ch);
            return (TRUE);
        }

        if (obj->obj_flags.value[2] != LIQ_WATER &&
            obj->obj_flags.value[1] != 0) {
            name_from_drinkcon(obj);
            obj->obj_flags.value[2] = LIQ_SLIME;
            name_to_drinkcon(obj, LIQ_SLIME);
        } else {
            /*
             * Calculate water it can contain
             */
            water = obj->obj_flags.value[0] - obj->obj_flags.value[1];

            if (water > 0) {
                obj->obj_flags.value[2] = LIQ_WATER;
                obj->obj_flags.value[1] += water;
                weight_change_object(obj, water);
                name_from_drinkcon(obj);
                name_to_drinkcon(obj, LIQ_WATER);
                act("$p is filled.", FALSE, ch, obj, 0, TO_CHAR);
                act("$n fills $p with water.", FALSE, ch, obj, 0, TO_ROOM);
            }
        }
        return (TRUE);
    }

    if (cmd == 11) {
        /*
         * drink
         */
        switch (ch->in_room) {
        case 13518:
        case 11014:
        case 5234:
        case 3141:
        case 13406:
            strncpy(container, "fountain", 20);
            break;
        case 22642:
        case 22644:
        case 22646:
        case 22648:
            strncpy(container, "brook", 20);
            break;
        case 53025:
            strncpy(container, "well", 20);
            break;
        default:
            strncpy(container, "fountain", 20);
        };

        only_argument(arg, buf);

        if (str_cmp(buf, container) && str_cmp(buf, "water")) {
            return (FALSE);
        }

        sprintf(buf, "You drink from the %s.\n\r", container);
        send_to_char(buf, ch);

        sprintf(buf, "$n drinks from the %s.", container);
        act(buf, FALSE, ch, 0, 0, TO_ROOM);

        if (!IS_IMMORTAL(ch)) {
            GET_COND(ch, THIRST) = 24;
        }
        if (GET_COND(ch, THIRST) > 20) {
            act("You do not feel thirsty.", FALSE, ch, 0, 0, TO_CHAR);
        }
        return (TRUE);
    }

    /*
     * All commands except fill and drink
     */
    return (FALSE);
}

int bank(struct char_data *ch, int cmd, char *arg, struct room_data *rp,
         int type)
{

    static char     buf[256];
    int             money;

    money = advatoi(arg);

    if (IS_NPC(ch)) {
        return (FALSE);
    }
    save_char(ch, ch->in_room);

    if (GET_BANK(ch) > GetMaxLevel(ch) * 40000 && GetMaxLevel(ch) < 40) {
        send_to_char("I'm sorry, but we can no longer hold more than 40000 "
                     "coins per level.\n\r", ch);
        GET_GOLD(ch) += GET_BANK(ch) - GetMaxLevel(ch) * 40000;
        GET_BANK(ch) = GetMaxLevel(ch) * 40000;
    }

    /*
     * deposit
     */
    if (cmd == 219) {
        if (HasClass(ch, CLASS_MONK) && (GetMaxLevel(ch) < 40)) {
            send_to_char
                ("Your vows forbid you to retain personal wealth\n\r", ch);
            return (TRUE);
        }

        if (money > GET_GOLD(ch)) {
            send_to_char("You don't have enough for that!\n\r", ch);
            return (TRUE);
        } else if (money <= 0) {
            send_to_char("Go away, you bother me.\n\r", ch);
            return (TRUE);
        } else if (money + GET_BANK(ch) > GetMaxLevel(ch) * 40000 &&
                   GetMaxLevel(ch) < 40) {
            send_to_char("I'm sorry, Regulations only allow us to ensure 40000"
                         " coins per level.\n\r", ch);
            return (TRUE);
        } else {
            send_to_char("Thank you.\n\r", ch);
            GET_GOLD(ch) = GET_GOLD(ch) - money;
            GET_BANK(ch) = GET_BANK(ch) + money;
            sprintf(buf, "Your balance is %d.\n\r", GET_BANK(ch));
            send_to_char(buf, ch);
            return (TRUE);
        }
    }

    if (cmd == 220) {
        /*
         * withdraw
         */
        if (HasClass(ch, CLASS_MONK) && GetMaxLevel(ch) < 40) {
            send_to_char("Your vows forbid you to retain personal wealth\n\r",
                         ch);
            return (TRUE);
        }

        if (money > GET_BANK(ch)) {
            send_to_char("You don't have enough in the bank for that!\n\r", ch);
            return (TRUE);
        } else if (money <= 0) {
            send_to_char("Go away, you bother me.\n\r", ch);
            return (TRUE);
        } else {
            send_to_char("Thank you.\n\r", ch);
            GET_GOLD(ch) = GET_GOLD(ch) + money;
            GET_BANK(ch) = GET_BANK(ch) - money;
            sprintf(buf, "Your balance is %d.\n\r", GET_BANK(ch));
            send_to_char(buf, ch);
            return (TRUE);
        }
    }

    if (cmd == 221) {
        sprintf(buf, "Your balance is %d.\n\r", GET_BANK(ch));
        send_to_char(buf, ch);
        return (TRUE);
    }
    return (FALSE);
}

/*
 * Idea of the LockSmith is functionally similar to the Pet Shop
 * The problem here is that each key must somehow be associated
 * with a certain player. My idea is that the players name will
 * appear as the another Extra description keyword, prefixed
 * by the words 'item_for_' and followed by the player name.
 * The (keys) must all be stored in a room which is (virtually)
 * adjacent to the room of the lock smith.
 */

int pray_for_items(struct char_data *ch, int cmd, char *arg,
                   struct room_data *rp, int type)
{
    char            buf[256];
    int             key_room,
                    gold;
    bool            found;
    struct obj_data *tmp_obj,
                   *obj;
    struct extra_descr_data *ext;

    if (cmd != 176) {
        /*
         * You must pray to get the stuff
         */
        return FALSE;
    }

    key_room = 1 + ch->in_room;

    strcpy(buf, "item_for_");
    strcat(buf, GET_NAME(ch));

    gold = 0;
    found = FALSE;

    for (tmp_obj = real_roomp(key_room)->contents; tmp_obj;
         tmp_obj = tmp_obj->next_content) {
        for (ext = tmp_obj->ex_description; ext; ext = ext->next) {
            if (str_cmp(buf, ext->keyword) == 0) {
                if (gold == 0) {
                    gold = 1;
                    act("$n kneels and at the altar and chants a prayer to "
                        "Odin.", FALSE, ch, 0, 0, TO_ROOM);
                    act("You notice a faint light in Odin's eye.",
                        FALSE, ch, 0, 0, TO_CHAR);
                }
                obj = read_object(tmp_obj->item_number, REAL);
                obj_to_room(obj, ch->in_room);
                act("$p slowly fades into existence.", FALSE, ch, obj, 0,
                    TO_ROOM);
                act("$p slowly fades into existence.", FALSE, ch, obj, 0,
                    TO_CHAR);
                gold += obj->obj_flags.cost;
                found = TRUE;
            }
        }
    }

    if (found) {
        GET_GOLD(ch) -= gold;
        GET_GOLD(ch) = MAX(0, GET_GOLD(ch));
        return TRUE;
    }

    return FALSE;
}

/*
 ********************************************************************
 *  Special procedures for objects                                     *
 ******************************************************************** */

#define CHAL_ACT \
"You are torn out of reality!\n\r"\
"You roll and tumble through endless voids for what seems like eternity...\n\r"\
"\n\r"\
"After a time, a new reality comes into focus... you are elsewhere.\n\r"

int chalice(struct char_data *ch, int cmd, char *arg)
{
    struct obj_data *chalice;
    char            buf1[MAX_INPUT_LENGTH],
                    buf2[MAX_INPUT_LENGTH];
    static int      chl = -1,
                    achl = -1;

    /*
     * 222 is the normal chalice, 223 is chalice-on-altar
     */

    if (chl < 1) {
        chl = real_object(222);
        achl = real_object(223);
    }

    switch (cmd) {
    case 10:
        /*
         * get
         */
        if (!(chalice =
                get_obj_in_list_num(chl, real_roomp(ch->in_room)-> contents)) &&
            CAN_SEE_OBJ(ch, chalice)) {
            if (!(chalice =
                    get_obj_in_list_num(achl,
                                        real_roomp(ch->in_room)->contents)) &&
                CAN_SEE_OBJ(ch, chalice)) {
                return (0);
            }
        }

        /*
         * we found a chalice.. now try to get us
         */
        do_get(ch, arg, cmd);

        /*
         * if got the altar one, switch her
         */
        if (chalice == get_obj_in_list_num(achl, ch->carrying)) {
            extract_obj(chalice);
            chalice = read_object(chl, VIRTUAL);
            obj_to_char(chalice, ch);
        }
        return (TRUE);
        break;
    case 67:
        /*
         * put
         */
        if (!(chalice = get_obj_in_list_num(chl, ch->carrying))) {
            return (FALSE);
        }
        argument_interpreter(arg, buf1, buf2);
        if (!str_cmp(buf1, "chalice") && !str_cmp(buf2, "altar")) {
            extract_obj(chalice);
            chalice = read_object(achl, VIRTUAL);
            obj_to_room(chalice, ch->in_room);
            send_to_char("Ok.\n\r", ch);
        }
        return (TRUE);
        break;
    case 176:
        /*
         * pray
         */
        if (!(chalice =
                get_obj_in_list_num(achl, real_roomp(ch->in_room)->contents))) {
            return (FALSE);
        }

        do_action(ch, arg, cmd);
        /*
         * pray
         */
        send_to_char(CHAL_ACT, ch);
        extract_obj(chalice);
        act("$n is torn out of existence!", TRUE, ch, 0, 0, TO_ROOM);
        char_from_room(ch);
        char_to_room(ch, 2500);
        /*
         * before the fiery gates
         */
        do_look(ch, "", 15);
        return (TRUE);
        break;
    default:
        return (FALSE);
        break;
    }
}

int kings_hall(struct char_data *ch, int cmd, char *arg)
{
    if (cmd != 176) {
        return (FALSE);
    }
    do_action(ch, arg, 176);

    send_to_char("You feel as if some mighty force has been offended.\n\r", ch);
    send_to_char(CHAL_ACT, ch);

    act("$n is struck by an intense beam of light and vanishes.",
        TRUE, ch, 0, 0, TO_ROOM);
    char_from_room(ch);
    char_to_room(ch, 1420);
    /*
     * behind the altar
     */
    do_look(ch, "", 15);
    return (TRUE);
}

/*
 * donation room
 */
int Donation(struct char_data *ch, int cmd, char *arg,
             struct room_data *rp, int type)
{
    char            arg1[40];
    char            arg2[40];
    struct obj_data *sub_object;
    
    if ((cmd != 10) && (cmd != 167)) {
        return (FALSE);
    }
    
    argument_interpreter(arg, arg1, arg2);

    if (*arg1 && !strncmp(arg1, "all", 3) && !*arg2) {
       
        /* removed check for immortal to test */
        send_to_char("Now now, that would be greedy!\n\r", ch);
        return (TRUE);
    }

    if (*arg2 && (sub_object = get_obj_vis_accessible(ch, arg2))) {
        if (GET_ITEM_TYPE(sub_object) == ITEM_CONTAINER) {
            if ((sub_object = get_obj_in_list_vis(ch, arg2, ch->carrying))) {
                return (FALSE);
            } else {
                send_to_char("Now now, that would be greedy!\n\r", ch);
                return (TRUE);
            }
        } else {
            send_to_char("That isnt a container.\n\r", ch);
            return (TRUE);
        }
    }
    return (FALSE);
}

/*
 * house routine for saved items.
 */

int House(struct char_data *ch, int cmd, char *arg, struct room_data *rp,
          int type)
{
    char            buf[100];
    struct obj_cost cost;
    int             i,
                    save_room;
    int             count = 0;

    if (IS_NPC(ch)) {
        return (FALSE);
    }
    /*
     * if (cmd != rent) ignore
     */
    if (cmd != 92) {
        return (FALSE);
    }

    /*
     * verify the owner
     */
    if (strncmp(GET_NAME(ch), real_roomp(ch->in_room)->name,
                strlen(GET_NAME(ch)))) {
        send_to_char("Sorry, you'll have to find your own house.\n\r", ch);
        return (FALSE);
    }

    cost.total_cost = 0;
    cost.no_carried = 0;
    cost.ok = TRUE;

    add_obj_cost(ch, 0, ch->carrying, &cost);
    count = CountLims(ch->carrying);
    for (i = 0; i < MAX_WEAR; i++) {
        add_obj_cost(ch, 0, ch->equipment[i], &cost);
        count += CountLims(ch->equipment[i]);
    }

    if (!cost.ok) {
        return (FALSE);
    }

#if LIMITED_ITEMS
    if (count > MaxLimited(GetMaxLevel(ch))) {
        send_to_char("I'm sorry, but you to many limited items.\n\r", ch);
        return (FALSE);
    }
#endif

    sprintf(buf, "It will cost you %d coins per day\n\r", cost.total_cost);
    send_to_char(buf, ch);

    save_obj(ch, &cost, 1);
    save_room = ch->in_room;

    if (ch->specials.start_room != 2) {
        ch->specials.start_room = save_room;
    }
    /*
     * CHARACTERS aren't freed by this
     */
    extract_char(ch);
    save_char(ch, save_room);
    ch->in_room = save_room;
    return( TRUE );
}

/***********************************************************************

                           CHESSBOARD PROCS

 ***********************************************************************/

#define SISYPHUS_MAX_LEVEL 9

/*
 * This is the highest level of PC that can enter.  The highest level
 * monster currently in the section is 14th.  It should require a fairly
 * large party to sweep the section.
 */

int sisyphus(struct char_data *ch, int cmd, char *arg,
             struct char_data *mob, int type)
{
    /*
     * use this as a switch, to avoid double challenges
     */
    static int      b = 1;

    if (cmd) {
        if (cmd <= 6 && cmd >= 1 && IS_PC(ch)) {
            if (b) {
                b = 0;
                do_look(mob, GET_NAME(ch), 0);
            } else {
                b = 1;
            }
            if (ch->in_room == Ivory_Gate && cmd == 4 &&
                SISYPHUS_MAX_LEVEL < GetMaxLevel(ch) &&
                GetMaxLevel(ch) < LOW_IMMORTAL) {
                if (!check_soundproof(ch)) {
                    act("$n tells you 'First you'll have to get past me!'",
                        TRUE, mob, 0, ch, TO_VICT);
                }
                act("$n grins evilly.", TRUE, mob, 0, ch, TO_VICT);
                return (TRUE);
            }
            return (FALSE);
        }

        /*
         * doorbash
         */
        if (cmd == 258) {
            act("$n tells you 'I don't think so.'", FALSE, mob, 0, ch, TO_VICT);
            return (TRUE);
        }

        return (FALSE);
    } else {
        if (ch->specials.fighting) {
            if (GET_POS(ch) < POSITION_FIGHTING &&
                GET_POS(ch) > POSITION_STUNNED) {
                StandUp(ch);
            } else {
                switch (number(1, 10)) {
                case 1:
                    do_say(ch, "heal", 0);
                    break;
                case 2:
                    do_say(ch, "pzar", 0);
                    break;
                default:
                    FighterMove(ch);
                    break;
                }
            }
            return (FALSE);
        }
    }
    return (FALSE);
}

int jabberwocky(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    if (cmd) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        if (GET_POS(ch) < POSITION_FIGHTING &&
            GET_POS(ch) > POSITION_STUNNED) {
            StandUp(ch);
        } else {
            FighterMove(ch);
        }
    }
    return (FALSE);
}

int flame(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
          int type)
{
    if (cmd) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        if (GET_POS(ch) < POSITION_FIGHTING &&
            GET_POS(ch) > POSITION_STUNNED) {
            StandUp(ch);
        } else {
            FighterMove(ch);
        }
    }
    return (FALSE);
}

int banana(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
           int type)
{
    if (!cmd) {
        return (FALSE);
    }
    if (cmd >= 1 && cmd <= 6 && GET_POS(ch) == POSITION_STANDING &&
        !IS_NPC(ch)) {
        if (!saves_spell(ch, SAVING_PARA)) {
            act("$N tries to leave, but slips on a banana and falls.",
                TRUE, ch, 0, ch, TO_NOTVICT);
            act("As you try to leave, you slip on a banana.",
                TRUE, ch, 0, ch, TO_VICT);
            GET_POS(ch) = POSITION_SITTING;
            return (TRUE);
        }
        return (FALSE);
    }
    return (FALSE);
}

int paramedics(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{
    struct char_data *vict,
                   *most_hurt;

    if (!cmd) {
        if (ch->specials.fighting) {
            return (cleric(ch, 0, "", mob, type));
        }

        if (GET_POS(ch) == POSITION_STANDING) {
            /*
             * Find a dude to do good things upon !
             */
            most_hurt = real_roomp(ch->in_room)->people;
            for (vict = real_roomp(ch->in_room)->people; vict;
                 vict = vict->next_in_room) {
                if (((float) GET_HIT(vict) / (float) hit_limit(vict) <
                     (float) GET_HIT(most_hurt) /
                     (float) hit_limit(most_hurt)) && (CAN_SEE(ch, vict))) {
                    most_hurt = vict;
                }
            }

            if (!most_hurt) {
                /*
                 * nobody here
                 */
                return (FALSE);
            }

            if ((float) GET_HIT(most_hurt) /
                (float) hit_limit(most_hurt) > 0.66) {
                if (number(0, 5) == 0) {
                    act("$n shrugs helplessly in unison.", 1, ch, 0, 0,
                        TO_ROOM);
                }
                /*
                 * not hurt enough
                 */
                return TRUE;
            }

            if (!check_soundproof(ch) && !number(0, 4)) {
                if (most_hurt != ch) {
                    act("$n looks at $N.", 1, ch, 0, most_hurt, TO_NOTVICT);
                    act("$n looks at you.", 1, ch, 0, most_hurt, TO_VICT);
                }

                if (check_nomagic(ch, 0, 0)) {
                    return (FALSE);
                }
                act("$n utters the words 'judicandus dies' in unison.", 1, ch,
                    0, 0, TO_ROOM);
                cast_cure_light(GetMaxLevel(ch), ch, "",
                                SPELL_TYPE_SPELL, most_hurt, 0);
                return (TRUE);
            }
        }
    }
    return (FALSE);
}

int jugglernaut(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    struct obj_data *tmp_obj;
    int             i,
                    j;

    if (cmd) {
        return (FALSE);
    }
    if (GET_POS(ch) == POSITION_STANDING) {
        if (number(0, 2)) {
            return FALSE;
        }
        /*
         * juggle something
         */

        if (IS_CARRYING_N(ch) < 1) {
            return FALSE;
        }
        i = number(0, IS_CARRYING_N(ch));
        j = 0;
        for (tmp_obj = ch->carrying; (tmp_obj) && (j < i); j++) {
            tmp_obj = tmp_obj->next_content;
        }

        if (number(0,5)) {
            if (number(0,1)) {
                act("$n tosses $p high into the air and catches it.", TRUE,
                    ch, tmp_obj, NULL, TO_ROOM);
            } else {
                act("$n sends $p whirling.", TRUE, ch, tmp_obj, NULL, TO_ROOM);
            }
        } else {
            act("$n tosses $p but fumbles it!", TRUE, ch, tmp_obj, NULL,
                TO_ROOM);
            obj_from_char(tmp_obj);
            obj_to_room(tmp_obj, ch->in_room);
        }
        return (TRUE);
    }
    return (FALSE);
}

char    *elf_comm[] = {
    "wake", "yawn",
    "stand", "say Well, back to work.", "get all",
    "eat bread", "wink",
    "w", "w", "s", "s", "s", "d", "open gate", "e",     /* home to gate */
    "close gate",
    "e", "e", "e", "e", "n", "w", "n",  /* gate to baker */
    "give all.bread baker",     /* pretend to give a bread */
    "give all.pastry baker",    /* pretend to give a pastry */
    "say That'll be 33 coins, please.",
    "echo The baker gives some coins to the Elf",
    "wave",
    "s", "e", "n", "n", "e", "drop all.bread", "drop all.pastry",
    "w", "s", "s",              /* to main square */
    "s", "w", "w", "w", "w",    /* back to gate */
    "pat sisyphus",
    "open gate", "w", "close gate", "u", "n", "n", "n", "e", "e", /* to home */
    "say Whew, I'm exhausted.", "rest", "$"
};

#define ELF_INIT     0
#define ELF_RESTING  1
#define ELF_GETTING  2
#define ELF_DELIVERY 3
#define ELF_DUMP 4
#define ELF_RETURN_TOWER   5
#define ELF_RETURN_HOME    6

int delivery_elf(struct char_data *ch, int cmd, char *arg,
                 struct char_data *mob, int type)
{
    int             dir;

    if (cmd) {
        return (FALSE);
    }
    return (FALSE);

    if (ch->specials.fighting) {
        return FALSE;
    }
    switch (ch->generic) {

    case ELF_INIT:
        if (ch->in_room != 0 && ch->in_room != Elf_Home) {
            if (GET_POS(ch) == POSITION_SLEEPING) {
                do_wake(ch, "", 0);
                do_stand(ch, "", 0);
            }
            do_say(ch, "Woah! How did i get here!", 0);
            do_emote(ch, "waves his arm, and vanishes!", 0);
            char_from_room(ch);
            char_to_room(ch, Elf_Home);

            do_emote(ch, "arrives with a Bamf!", 0);
            do_emote(ch, "yawns", 0);
            do_sleep(ch, "", 0);
        }
        ch->generic = ELF_RESTING;
        return (FALSE);
        break;

    case ELF_RESTING:
        if ((time_info.hours > 6) && (time_info.hours < 9)) {
            do_wake(ch, "", 0);
            do_stand(ch, "", 0);
            ch->generic = ELF_GETTING;
        }
        return (FALSE);
        break;

    case ELF_GETTING:
        do_get(ch, "all.loaf", 0);
        do_get(ch, "all.biscuit", 0);
        ch->generic = ELF_DELIVERY;
        return (FALSE);
        break;
    case ELF_DELIVERY:
        if (ch->in_room != Bakery) {
            dir = choose_exit_global(ch->in_room, Bakery, -100);
            if (dir < 0) {
                ch->generic = ELF_INIT;
                return (FALSE);
            } else {
                go_direction(ch, dir);
            }
        } else {
            do_give(ch, "6*biscuit baker", 0);
            do_give(ch, "6*loaf baker", 0);
            do_say(ch, "That'll be 33 coins, please.", 0);
            ch->generic = ELF_DUMP;
        }
        return (FALSE);
        break;
    case ELF_DUMP:
        if (ch->in_room != Dump) {
            dir = choose_exit_global(ch->in_room, Dump, -100);
            if (dir < 0) {
                ch->generic = ELF_INIT;
                return (FALSE);
            } else {
                go_direction(ch, dir);
            }
        } else {
            do_drop(ch, "10*biscuit", 0);
            do_drop(ch, "10*loaf", 0);
            ch->generic = ELF_RETURN_TOWER;
        }
        return (FALSE);
        break;
    case ELF_RETURN_TOWER:
        if (ch->in_room != Ivory_Gate) {
            dir = choose_exit_global(ch->in_room, Ivory_Gate, -200);
            if (dir < 0) {
                ch->generic = ELF_INIT;
                return (FALSE);
            } else {
                go_direction(ch, dir);
            }
        } else {
            ch->generic = ELF_RETURN_HOME;
        }
        return (FALSE);
        break;
    case ELF_RETURN_HOME:
        if (ch->in_room != Elf_Home) {
            dir = choose_exit_global(ch->in_room, Elf_Home, -200);
            if (dir < 0) {
                ch->generic = ELF_INIT;
                return (FALSE);
            } else {
                go_direction(ch, dir);
            }
        } else if (time_info.hours > 21) {
            do_say(ch, "Done at last!", 0);
            do_sleep(ch, "", 0);
            ch->generic = ELF_RESTING;
        } else {
            do_say(ch, "An elf's work is never done.", 0);
            ch->generic = ELF_GETTING;
        }
        return (FALSE);
        break;
    default:
        ch->generic = ELF_INIT;
        return (FALSE);
    }
}

int delivery_beast(struct char_data *ch, int cmd, char *arg,
                   struct char_data *mob, int type)
{
    struct obj_data *o;

    if (cmd) {
        return (FALSE);
    }
    return (FALSE);

    if (time_info.hours == 6) {
        do_drop(ch, "all.loaf", 0);
        do_drop(ch, "all.biscuit", 0);
    } else if (time_info.hours < 2) {
        if (!number(0, 1)) {
            o = read_object(3012, VIRTUAL);
            obj_to_char(o, ch);
        } else {
            o = read_object(3013, VIRTUAL);
            obj_to_char(o, ch);
        }
    } else if (GET_POS(ch) > POSITION_SLEEPING) {
        do_sleep(ch, "", 0);
    }
}

int Keftab(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
           int type)
{
    int             found,
                    targ_item;
    struct char_data *i;

    if (cmd) {
        return (FALSE);
    }
    if (!ch->specials.hunting) {
        /*
         * find a victim
         */

        for (i = character_list; i; i = i->next) {
            if (!IS_NPC(ch)) {
                targ_item = SWORD_ANCIENTS;
                found = FALSE;
                while (!found) {
                    if ((HasObject(i, targ_item)) && (GetMaxLevel(i) < 30)) {
                        AddHated(ch, i);
                        SetHunting(ch, i);
                        return (TRUE);
                    } else {
                        targ_item++;
                        if (targ_item > SWORD_ANCIENTS + 20)
                            found = TRUE;
                    }
                }
            }
        }
        return (FALSE);
    }

    /*
     * check to make sure that the victim still has an item
     */
    found = FALSE;
    targ_item = SWORD_ANCIENTS;
    while (!found) {
        if (HasObject(ch->specials.hunting, targ_item)) {
            return (FALSE);
        } else {
            targ_item++;
            if (targ_item == SWORD_ANCIENTS + 20)
                found = FALSE;
        }
        ch->specials.hunting = 0;
        found = TRUE;
    }
    return (FALSE);
}

int StormGiant(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{
    struct char_data *vict;

    if (cmd) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        if ((GET_POS(ch) < POSITION_FIGHTING) &&
            (GET_POS(ch) > POSITION_STUNNED)) {
            StandUp(ch);
        } else {
            if (number(0, 5)) {
                fighter(ch, cmd, arg, mob, type);
            } else {
                act("$n creates a lightning bolt", TRUE, ch, 0, 0,
                    TO_ROOM);
                if ((vict = FindAHatee(ch)) == NULL) {
                    vict = FindVictim(ch);
                }
                if (!vict) {
                    return (FALSE);
                }
                cast_lightning_bolt(GetMaxLevel(ch), ch, "",
                                    SPELL_TYPE_SPELL, vict, 0);
            }
        }
    }
    return (FALSE);
}

int fighter(struct char_data *ch, int cmd, char *arg,
            struct char_data *mob, int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        if (GET_POS(ch) == POSITION_FIGHTING) {
            FighterMove(ch);
        } else {
            StandUp(ch);
        }
        FindABetterWeapon(ch);
    }
    return (FALSE);
}

/*
 **  NEW THALOS MOBS:******************************************************
 */

#define NTMOFFICE  13554
#define NTMNGATE   13422
#define NTMEGATE   13431
#define NTMSGATE   13413
#define NTMWGATE   13423

#define NTMWMORN    0
#define NTMSTARTM   1
#define NTMGOALNM   2
#define NTMGOALEM   3
#define NTMGOALSM   4
#define NTMGOALWM   5
#define NTMGOALOM   6
#define NTMWNIGHT   7
#define NTMSTARTN   8
#define NTMGOALNN   9
#define NTMGOALEN   10
#define NTMGOALSN   11
#define NTMGOALWN   12
#define NTMGOALON   13
#define NTMSUSP     14
#define NTM_FIX     15

int NewThalosMayor(struct char_data *ch, int cmd, char *arg,
                   struct char_data *mob, int type)
{
    int             dir;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (!ch->specials.fighting) {
        /*
         * state info
         */
        switch (ch->generic) {
        case NTMWMORN:
            /*
             * wait for morning
             */
            if (time_info.hours == 6) {
                ch->generic = NTMGOALNM;
                return (FALSE);
            }
            break;
        case NTMGOALNM:
            /*
             * north gate
             */
            if (ch->in_room != NTMNGATE) {
                dir = choose_exit_global(ch->in_room, NTMNGATE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                /*
                 * unlock and open door.
                 */
                do_unlock(ch, " gate", 0);
                do_open(ch, " gate", 0);
                ch->generic = NTMGOALEM;
            }
            return (FALSE);
            break;
        case NTMGOALEM:
            if (ch->in_room != NTMEGATE) {
                dir = choose_exit_global(ch->in_room, NTMEGATE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                /*
                 * unlock and open door.
                 */
                do_unlock(ch, " gate", 0);
                do_open(ch, " gate", 0);
                ch->generic = NTMGOALSM;
            }
            return (FALSE);
            break;
        case NTMGOALSM:
            if (ch->in_room != NTMSGATE) {
                dir = choose_exit_global(ch->in_room, NTMSGATE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                /*
                 * unlock and open door.
                 */
                do_unlock(ch, " gate", 0);
                do_open(ch, " gate", 0);
                ch->generic = NTMGOALWM;
            }
            return (FALSE);
            break;
        case NTMGOALWM:
            if (ch->in_room != NTMWGATE) {
                dir = choose_exit_global(ch->in_room, NTMWGATE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                /*
                 * unlock and open door.
                 */
                do_unlock(ch, " gate", 0);
                do_open(ch, " gate", 0);
                ch->generic = NTMGOALOM;
            }
            return (FALSE);
            break;
        case NTMGOALOM:
            if (ch->in_room != NTMOFFICE) {
                dir = choose_exit_global(ch->in_room, NTMOFFICE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                ch->generic = NTMWNIGHT;
            }
            return (FALSE);
            break;
        case NTMWNIGHT:
            /*
             * go back to wait for 7pm
             */
            if (time_info.hours == 19) {
                ch->generic = NTMGOALNN;
            }
        case NTMGOALNN:
            /*
             * north gate
             */
            if (ch->in_room != NTMNGATE) {
                dir = choose_exit_global(ch->in_room, NTMNGATE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                /*
                 * lock and open door.
                 */
                do_lock(ch, " gate", 0);
                do_close(ch, " gate", 0);
                ch->generic = NTMGOALEN;
            }
            return (FALSE);
            break;
        case NTMGOALEN:
            if (ch->in_room != NTMEGATE) {
                dir = choose_exit_global(ch->in_room, NTMEGATE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                /*
                 * lock and open door.
                 */
                do_lock(ch, " gate", 0);
                do_close(ch, " gate", 0);
                ch->generic = NTMGOALSN;
            }
            return (FALSE);
            break;
        case NTMGOALSN:
            if (ch->in_room != NTMSGATE) {
                dir = choose_exit_global(ch->in_room, NTMSGATE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                /*
                 * lock and open door.
                 */
                do_lock(ch, " gate", 0);
                do_close(ch, " gate", 0);
                ch->generic = NTMGOALWN;
            }
            return (FALSE);
            break;
        case NTMGOALWN:
            if (ch->in_room != NTMWGATE) {
                dir = choose_exit_global(ch->in_room, NTMWGATE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                /*
                 * unlock and open door.
                 */
                do_lock(ch, " gate", 0);
                do_close(ch, " gate", 0);
                ch->generic = NTMGOALOM;
            }
            return (FALSE);
            break;
        case NTMGOALON:
            if (ch->in_room != NTMOFFICE) {
                dir = choose_exit_global(ch->in_room, NTMOFFICE, -100);
                if (dir < 0) {
                    ch->generic = NTM_FIX;
                    return (FALSE);
                } else {
                    go_direction(ch, dir);
                }
            } else {
                ch->generic = NTMWMORN;
            }
            return (FALSE);
            break;
        case NTM_FIX:
            /*
             * move to correct spot (office)
             */
            do_say(ch, "Woah! How did i get here!", 0);
            char_from_room(ch);
            char_to_room(ch, NTMOFFICE);
            ch->generic = NTMWMORN;
            return (FALSE);
            break;
        default:
            ch->generic = NTM_FIX;
            return (FALSE);
            break;
        }
    }
    return( FALSE );
}

int SultanGuard(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    return (GenericCityguard(ch, cmd, arg, mob, NEWTHALOS));
}

int NewThalosCitizen(struct char_data *ch, int cmd, char *arg,
                     struct char_data *mob, int type)
{
    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        fighter(ch, cmd, arg, mob, type);

        if (!check_soundproof(ch)) {
            if (number(0, 18) == 0) {
                do_shout(ch, "Guards! Help me! Please!", 0);
            } else {
                act("$n shouts 'Guards!  Help me! Please!'", TRUE, ch, 0,
                    0, TO_ROOM);
            }

            if (ch->specials.fighting) {
                CallForGuard(ch, ch->specials.fighting, 3, NEWTHALOS);
            }
            return (TRUE);
        }
    }

    return (FALSE);
}

int NewThalosGuildGuard(struct char_data *ch, int cmd, char *arg,
                        struct char_data *mob, int type)
{
    if (!cmd) {
        if (ch->specials.fighting) {
            return (fighter(ch, cmd, arg, mob, type));
        }
    } else {
        if (cmd >= 1 && cmd <= 6) {
            switch (ch->in_room) {
            case 13532:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 13532, 2, CLASS_THIEF));
                break;
            case 13512:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 13512, 2, CLASS_CLERIC));
                break;
            case 13526:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 13526, 2, CLASS_WARRIOR));
                break;
            case 13525:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 13525, 0,
                         CLASS_MAGIC_USER | CLASS_SORCERER));
                break;
            }
        }
    }
    return (FALSE);
}

/*
 * New improved magic_user
 */

int magic_user2(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    struct char_data *vict;
    byte            lspell;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (!ch->specials.fighting) {
        return FALSE;
    }
    if (GET_POS(ch) > POSITION_STUNNED && GET_POS(ch) < POSITION_FIGHTING) {
        StandUp(ch);
        return (TRUE);
    }

    if (check_nomagic(ch, 0, 0)) {
        return (FALSE);
    }
    vict = FindVictim(ch);

    if (!vict) {
        vict = ch->specials.fighting;
    }
    if (!vict) {
        return (FALSE);
    }
    lspell = number(0, GetMaxLevel(ch));

    if (lspell < 1) {
        lspell = 1;
    }
    if (vict != ch->specials.fighting && lspell > 19 &&
        !IS_AFFECTED(ch, AFF_FIRESHIELD)) {
        act("$n utters the words 'Fireshield'.", 1, ch, 0, 0, TO_ROOM);
        cast_fireshield(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, ch, 0);
        return TRUE;
    }

    switch (lspell) {
    case 1:
        act("$n utters the words 'Magic Missile'.", 1, ch, 0, 0, TO_ROOM);
        cast_magic_missile(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 2:
        act("$n utters the words 'Shocking Grasp'.", 1, ch, 0, 0, TO_ROOM);
        cast_shocking_grasp(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 3:
    case 4:
        act("$n utters the words 'Chill Touch'.", 1, ch, 0, 0, TO_ROOM);
        cast_chill_touch(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 5:
        act("$n utters the words 'Burning Hands'.", 1, ch, 0, 0, TO_ROOM);
        cast_burning_hands(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 6:
        if (!IS_AFFECTED(vict, AFF_SANCTUARY)) {
            act("$n utters the words 'Dispel Magic'.", 1, ch, 0, 0, TO_ROOM);
            cast_dispel_magic(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                              vict, 0);
        } else {
            act("$n utters the words 'Chill Touch'.", 1, ch, 0, 0, TO_ROOM);
            cast_chill_touch(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                             vict, 0);
        }
        break;
    case 7:
        act("$n utters the words 'Ice Storm'.", 1, ch, 0, 0, TO_ROOM);
        cast_ice_storm(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 8:
        act("$n utters the words 'Blindness'.", 1, ch, 0, 0, TO_ROOM);
        cast_blindness(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 9:
        act("$n utters the words 'Fear'.", 1, ch, 0, 0, TO_ROOM);
        cast_fear(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 10:
    case 11:
        act("$n utters the words 'Lightning Bolt'.", 1, ch, 0, 0, TO_ROOM);
        cast_lightning_bolt(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL,
                            vict, 0);
        break;
    case 12:
    case 13:
        act("$n utters the words 'Color Spray'.", 1, ch, 0, 0, TO_ROOM);
        cast_colour_spray(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 14:
        act("$n utters the words 'Cone Of Cold'.", 1, ch, 0, 0, TO_ROOM);
        cast_cone_of_cold(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
        act("$n utters the words 'Fireball'.", 1, ch, 0, 0, TO_ROOM);
        cast_fireball(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;

    default:
        act("$n utters the words 'frag'.", 1, ch, 0, 0, TO_ROOM);
        cast_meteor_swarm(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;

    }

    return TRUE;
}

/******************Mordilnia citizens************************************/

int MordGuard(struct char_data *ch, int cmd, char *arg,
              struct char_data *mob, int type)
{
    return (GenericCityguardHateUndead(ch, cmd, arg, mob, MORDILNIA));
}

int MordGuildGuard(struct char_data *ch, int cmd, char *arg,
                   struct char_data *mob, int type)
{
    if (!cmd) {
        if (ch->specials.fighting) {
            return (fighter(ch, cmd, arg, mob, type));
        }
    } else {
        if (cmd >= 1 && cmd <= 6) {
            switch (ch->in_room) {
            case 18266:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 18266, 2,
                         CLASS_MAGIC_USER | CLASS_SORCERER));
                break;
            case 18276:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 18276, 2, CLASS_CLERIC));
                break;
            case 18272:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 18272, 2, CLASS_THIEF));
                break;
            case 18256:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 18256, 0, CLASS_WARRIOR));
                break;
            }
        }
    }
    return (FALSE);
}

int CaravanGuildGuard(struct char_data *ch, int cmd, char *arg,
                      struct char_data *mob, int type)
{
    if (!cmd) {
        if (ch->specials.fighting) {
            return (fighter(ch, cmd, arg, mob, type));
        }
    } else {
        if (cmd >= 1 && cmd <= 6) {
            switch (ch->in_room) {
            case 16115:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 16115, 1,
                         CLASS_MAGIC_USER | CLASS_SORCERER));
                break;
            case 16126:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 16116, 1, CLASS_CLERIC));
                break;
            case 16117:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 16117, 3, CLASS_THIEF));
                break;
            case 16110:
                return (CheckForBlockedMove
                        (ch, cmd, arg, 16110, 3, CLASS_WARRIOR));
                break;
            }
        }
    }
    return (FALSE);
}

int StatTeller(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{
    int             choice;
    char            buf[200];
    extern struct str_app_type str_app[];

    if (cmd) {
        if (cmd == 56) {
            /*
             * buy
             *
             * for Ash's dam/hitroll teller
             */

            if (mob_index[mob->nr].virtual == 31818) {
                if (GET_GOLD(ch) < 10000) {
                    act("$c0013[$c0015$N$c0013] tells you 'You do not have the"
                        " money to pay me.\n\r", FALSE, ch, 0, mob, TO_CHAR);
                    return (TRUE);
                } else {
                    GET_GOLD(ch) -= 10000;
                }

                choice = number(0, 1);
                if (choice == 0) {
                    sprintf(buf, "$c0013[$c0015$N$c0013] tells you 'I sense "
                                 "your damroll is about %d.'",
                            ch->points.damroll +
                            str_app[STRENGTH_APPLY_INDEX(ch)].todam );
                } else {
                    sprintf(buf, "$c0013[$c0015$N$c0013] tells you 'I sense "
                                 "your hitroll is about %d.'",
                            ch->points.hitroll +
                            str_app[STRENGTH_APPLY_INDEX(ch)].tohit);
                }

                act(buf, FALSE, ch, 0, mob, TO_CHAR);
                act("$n gives some gold to $N", FALSE, ch, 0, mob, TO_ROOM);
                act("You give $N some gold.", FALSE, ch, 0, mob, TO_CHAR);
                return (TRUE);
            }

            /*
             * randomly tells a player 3 of his/her stats.. for a price
             */
            if (GET_GOLD(ch) < 1000) {
                send_to_char("You do not have the money to pay me.\n\r", ch);
                return (TRUE);
            } else {
                GET_GOLD(ch) -= 1000;
            }

            choice = number(0, 2);
            switch (choice) {
            case 0:
                sprintf(buf, "STR: %d, WIS: %d, DEX: %d\n\r", GET_STR(ch),
                        GET_WIS(ch), GET_DEX(ch));
                send_to_char(buf, ch);
                break;
            case 1:
                sprintf(buf, "INT: %d, DEX:  %d, CON: %d \n\r",
                        GET_INT(ch), GET_DEX(ch), GET_CON(ch));
                send_to_char(buf, ch);
                break;
            case 2:
                sprintf(buf, "CON: %d, INT: %d , WIS: %d \n\r",
                        GET_CON(ch), GET_INT(ch), GET_WIS(ch));
                send_to_char(buf, ch);
                break;
            default:
                send_to_char("We are experiencing Technical difficulties\n\r",
                             ch);
                return (TRUE);
            }
            return (TRUE);
        }
    } else {
        /*
         * in combat, issues a more potent curse.
         */
        if (ch->specials.fighting) {
            act("$n gives you the evil eye!  You feel your hitpoints ebbing "
                "away", FALSE, ch, 0, ch->specials.fighting, TO_VICT);
            act("$n gives $N the evil eye!  $N seems weaker!",
                FALSE, ch, 0, ch->specials.fighting, TO_NOTVICT);
            ch->specials.fighting->points.max_hit -= 10;
            ch->specials.fighting->points.hit -= 10;
        }
    }
    return (FALSE);
}

void ThrowChar(struct char_data *ch, struct char_data *v, int dir)
{
    struct room_data *rp;
    int             or;
    char            buf[200];

    rp = real_roomp(v->in_room);
    if (rp && rp->dir_option[dir] && rp->dir_option[dir]->to_room &&
        EXIT(v, dir)->to_room != NOWHERE) {
        if (v->specials.fighting) {
            stop_fighting(v);
        }

        sprintf(buf, "%s picks you up and throws you %s\n\r",
                ch->player.short_descr, dirs[dir]);
        send_to_char(buf, v);

        or = v->in_room;
        char_from_room(v);
        char_to_room(v, (real_roomp(or))->dir_option[dir]->to_room);
        do_look(v, "\0", 15);

        if (IS_SET(RM_FLAGS(v->in_room), DEATH) &&
            GetMaxLevel(v) < LOW_IMMORTAL) {
            NailThisSucker(v);
        }
    }
}

int ThrowerMob(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{
    struct char_data *vict;

    /*
     * Throws people in various directions
     */

    if (!cmd) {
        if (AWAKE(ch) && ch->specials.fighting) {
            /*
             * take this person and throw them
             */
            vict = ch->specials.fighting;
            switch (ch->in_room) {
            case 13912:
                ThrowChar(ch, vict, 1);
                /*
                 * throw chars to the east
                 */
                break;
            default:
                break;
            }
        }
    } else {
        switch (ch->in_room) {
        case 13912:
            if (cmd == 1) {
                /*
                 * north+1
                 */
                send_to_char("The Troll blocks your way.\n", ch);
                return (TRUE);
            }
            break;
        default:
            break;
        }
    }
    return (FALSE);
}


/*
 * Swallower special
 */

int Tyrannosaurus_swallower(struct char_data *ch, int cmd, char *arg,
                            struct char_data *mob, int type)
{
    struct obj_data *co,
                   *o;
    struct char_data *targ;
    struct room_data *rp;
    int             i,
                    index;
    char            buf[256];

    if (cmd && cmd != 156) {
        return (FALSE);
    }
    if (cmd == 156) {
        send_to_char("You're much too afraid to steal anything!\n\r", ch);
        return (TRUE);
    }

#if 0
    /*
     ** damage stuff

     DestroyedItems = 0;

     DamageStuff(ch, SPELL_ACID_BLAST, 100);

     if (DestroyedItems) {
     act("$n lets off a real rip-roarer!", FALSE, ch, 0, 0, TO_ROOM);
     DestroyedItems = 0;
     }

     Hrm... those mobs do not really need to scrap EVERYTHING they have :P */
#endif

    /*
     * swallow
     */
    if (AWAKE(ch) && (targ = FindAnAttacker(ch)) != '\0') {
        act("$n opens $s gaping mouth", TRUE, ch, 0, 0, TO_ROOM);
        if (!CAN_SEE(ch, targ) && saves_spell(targ, SAVING_PARA)) {
            act("$N barely misses being swallowed whole!",
                FALSE, ch, 0, targ, TO_NOTVICT);
            act("You barely avoid being swallowed whole!",
                FALSE, ch, 0, targ, TO_VICT);
            return (FALSE);
        }

        if (!saves_spell(targ, SAVING_PARA)) {
            act("In a single gulp, $N is swallowed whole!",
                TRUE, ch, 0, targ, TO_ROOM);
            send_to_char("In a single gulp, you are swallowed whole!\n\r",
                 targ);
            send_to_char("The horror!  The horror!\n\r", targ);
            send_to_char("MMM.  yum!\n\r", ch);
            /*
             * kill target:
             */
            GET_HIT(targ) = 0;
            sprintf(buf, "%s killed by being swallowed whole", GET_NAME(targ));
            Log(buf);
            die(targ, '\0');

            /*
             * all stuff to monster: this one is tricky.  assume that
             * corpse is top item on item_list now that corpse has
             * been made.
             */
            rp = real_roomp(ch->in_room);
            if (!rp)
                return (FALSE);

            for (co = rp->contents; co; co = co->next_content) {
                if (IS_CORPSE(co)) {
                    /*
                     * assume 1st corpse is victim's
                     */
                    while (co->contains) {
                        o = co->contains;
                        obj_from_obj(o);
                        obj_to_char(o, ch);

                        if (ITEM_TYPE(o) == ITEM_POTION) {
                            /*
                             * do the effects of the potion:
                             */
                            for (i = 1; i < 4; i++) {
                                if (o->obj_flags.value[i] >= 1) {
                                    index = spell_index[o->obj_flags.value[i]];
                                    if( spell_info[index].spell_pointer ) {
                                        ((*spell_info[index].spell_pointer)
                                         ((byte)o->obj_flags.value[0], ch,
                                          "", SPELL_TYPE_POTION, ch, o));
                                    }
                                }
                            }
                            extract_obj(o);

                        }
                    }
                    extract_obj(co);
                    return (TRUE);
                }
            }
        } else {
            act("$N barely misses being swallowed whole!", FALSE, ch,
                0, targ, TO_NOTVICT);
            act("You barely avoid being swallowed whole!", FALSE, ch,
                0, targ, TO_VICT);
        }
    }
    return( FALSE );
}

int soap(struct char_data *ch, int cmd, char *arg, struct obj_data *tobj,
         int type)
{
    struct char_data *t;
    struct obj_data *obj;
    char            dummy[80],
                    name[80];
    int             (*wash) ();

    wash = soap;

    if (type != PULSE_COMMAND) {
        return (FALSE);
    }
    if (cmd != 172) {
        return (FALSE);
    }
    if (!(obj = ch->equipment[HOLD])) {
        return (FALSE);
    }
    if (obj_index[obj->item_number].func != wash) {
        return (FALSE);
    }
    arg = one_argument(arg, dummy);
    if (!(*dummy)) {
        return (FALSE);
    }
    only_argument(arg, name);
    if (!(*name)) {
        return (FALSE);
    }
    if (!(t = get_char_room_vis(ch, name))) {
        return (FALSE);
    }
    if (affected_by_spell(t, SPELL_WEB)) {
        affect_from_char(t, SPELL_WEB);
        act("$n washes some webbing off $N with $p.", TRUE, ch, obj, t,
            TO_ROOM);
        act("You wash some webbing off $N with $p.", FALSE, ch, obj, t,
            TO_CHAR);
    } else {
        act("$n gives $N a good lathering with $p.", TRUE, ch, obj, t,
            TO_ROOM);
        act("You give $N a good lathering with $p.", FALSE, ch, obj, t,
            TO_CHAR);
    }

    obj->obj_flags.value[0]--;
    if (!obj->obj_flags.value[0]) {
        act("That used up $p.", FALSE, ch, obj, t, TO_CHAR);
        extract_obj(obj);
    }
    return( TRUE );
}

int nodrop(struct char_data *ch, int cmd, char *arg, struct obj_data *tobj,
           int type)
{
    struct char_data *t = NULL;
    struct obj_data *obj,
                   *i;
    char            buf[80],
                    obj_name[80],
                    vict_name[80],
                   *name;
    bool            do_all;
    int             j,
                    num;
    int             (*knowdrop) ();

    switch (cmd) {
    case 10:                    /* Get */
    case 60:                    /* Drop */
    case 72:                    /* Give */
    case 156:                   /* Steal */
        break;
    default:
        return (FALSE);
    }

    knowdrop = nodrop;

    if (type != PULSE_COMMAND) {
        return (FALSE);
    }
    arg = one_argument(arg, obj_name);
    if (!*obj_name) {
        return (FALSE);
    }
    obj = 0x0;
    do_all = FALSE;

    if (!strncmp(obj_name, "all", 3)) {
        do_all = TRUE;
        num = IS_CARRYING_N(ch);
    } else {
        strcpy(buf, obj_name);
        name = buf;
        if (!(num = get_number(&name))) {
            return (FALSE);
        }
    }

    /*
     * Look in the room first, in get case
     */
    if (cmd == 10) {
        for (i = real_roomp(ch->in_room)->contents, j = 1; i && (j <= num);
             i = i->next_content) {
            if (i->item_number >= 0 && (do_all || isname(name, i->name))) {
                if (do_all || j == num) {
                    if (obj_index[i->item_number].func == knowdrop) {
                        obj = i;
                        break;
                    }
                } else {
                    ++j;
                }
            }
        }
    }

    /*
     * Check the character's inventory for give, drop, steal.
     */
    if (!obj && cmd == 10) {
        /*
         * Don't bother with get anymore
         */
        return (FALSE);
    }

    for (i = ch->carrying, j = 1; i && (j <= num); i = i->next_content) {
        if (i->item_number >= 0 && (do_all || isname(name, i->name))) {
            if (do_all || j == num) {
                if (obj_index[i->item_number].func == knowdrop) {
                    obj = i;
                    break;
                } else if (!do_all) {
                    return (FALSE);
                }
            } else {
                ++j;
            }
        }
    }

    /*
     * Musta been something else
     */
    if (!obj) {
        return (FALSE);
    }
    if (cmd == 72 || cmd == 156) {
        only_argument(arg, vict_name);
        if (!*vict_name || !(t = get_char_room_vis(ch, vict_name))) {
            return (FALSE);
        }
    }

    switch (cmd) {

    case 10:
        if (GetMaxLevel(ch) <= MAX_MORT) {
            act("$p disintegrates when you try to pick it up!",
                FALSE, ch, obj, 0, TO_CHAR);
            act("$n tries to get $p, but it disintegrates in his hand!",
                FALSE, ch, obj, 0, TO_ROOM);
            extract_obj(obj);
            if (do_all) {
                return (FALSE);
            } else {
                return (TRUE);
            }
        } else {
            return (FALSE);
        }
        break;
    case 60:
        if (!IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
            act("You drop $p to the ground, and it shatters!",
                FALSE, ch, obj, 0, TO_CHAR);
            act("$n drops $p, and it shatters!", FALSE, ch, obj, 0, TO_ROOM);

            i = read_object(30, VIRTUAL);
            sprintf(buf, "Scraps from %s lie in a pile here.",
                    obj->short_description);
            i->description = (char *) strdup(buf);
            obj_to_room(i, ch->in_room);
            obj_from_char(obj);
            extract_obj(obj);

            if (do_all) {
                return (FALSE);
            } else {
                return (TRUE);
            }
        } else {
            return (FALSE);
        }
        break;

    case 72:
        if (!IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
            if (GetMaxLevel(ch) <= MAX_MORT) {
                act("You try to give $p to $N, but it vanishes!",
                    FALSE, ch, obj, t, TO_CHAR);
                act("$N tries to give $p to you, but it fades away!",
                    FALSE, t, obj, ch, TO_CHAR);
                act("As $n tries to give $p to $N, it vanishes!",
                    FALSE, ch, obj, t, TO_ROOM);
                extract_obj(obj);
                if (do_all) {
                    return (FALSE);
                } else {
                    return (TRUE);
                }
            } else {
                return (FALSE);
            }
        } else {
            return (FALSE);
        }
        break;

    case 156:
        /*
         * Steal
         */
        if (!IS_SET(obj->obj_flags.extra_flags, ITEM_NODROP)) {
            act("You cannot seem to steal $p from $N.",
                FALSE, ch, obj, t, TO_CHAR);
            act("$N tried to steal something from you!", FALSE, t, obj, ch,
                TO_CHAR);
            act("$N tried to steal something from $n!", FALSE, t, obj, ch,
                TO_ROOM);
            return (TRUE);
        } else {
            return (FALSE);
        }
        break;

    default:
        return (FALSE);
        break;
    }

    return (FALSE);
}

char           *lattimore_descs[] = {
    "A small orc is trying to break into a locker.\n\r",
    "A small orc is walking purposefully down the hall.\n\r",
    "An orc is feeding it's face with rat stew.\n\r",
    "A small orc is cowering underneath a bunk\n\r",
    "An orc sleeps restlessly on a bunk.\n\r",
    "There is an orc stading on a barrel here.\n\r",
    "An orc is traveling down the corridor at high speed.\n\r"
};

#define Lattimore_Initialize  0
#define Lattimore_Lockers     1
#define Lattimore_FoodRun     2
#define Lattimore_Eating      3
#define Lattimore_GoHome      4
#define Lattimore_Hiding      5
#define Lattimore_Sleeping    6
#define Lattimore_Run         7
#define Lattimore_Item        8

#define Kitchen   21310
#define Barracks  21277
#define Storeroom 21319
#define Conf      21322
#define Trap      21335
#define EarthQ    21334

#define CrowBar   21114
#define PostKey   21150


int lattimore(struct char_data *ch, int cmd, char *arg,
              struct char_data *mob, int type)
{
    struct memory  *mem;
    struct char_data *latt,
                   *t;
    struct obj_data *obj;
    char            obj_name[80],
                    player_name[80];
    int             dir;
    int             (*Lattimore) ();

    if (!cmd) {
        if (!ch->act_ptr) {
            ch->act_ptr = (struct memory *) malloc(sizeof(struct memory));
            mem = ch->act_ptr;
            mem->pointer = mem->c = mem->index = 0;
        } else {
            mem = (void *) ch->act_ptr;
        }
        if (ch->master) {
            mem->pointer = 0;
            return (FALSE);
        }
        if (!AWAKE(ch)) {
            return (FALSE);
        }
        if (ch->specials.fighting) {
            if (!IS_MOB(ch->specials.fighting) &&
                CAN_SEE(ch, ch->specials.fighting)) {
                affect_status(mem, ch, ch->specials.fighting, -5);
            }

            if (mem->status[mem->index] < 0) {
                strcpy(ch->player.long_descr, lattimore_descs[6]);
                mem->pointer = Lattimore_Run;
            } else if (mem->status[mem->index] > 19)
                mem->pointer = Lattimore_Item;
            return (FALSE);
        }

        /*
         * This case is used at startup, and after player interaction
         */
        switch (mem->pointer) {
        case Lattimore_Initialize:
            if (time_info.hours < 5 || time_info.hours > 21) {
                strcpy(ch->player.long_descr, lattimore_descs[3]);
                if (ch->in_room != Barracks) {
                    char_from_room(ch);
                    char_to_room(ch, Barracks);
                }
                mem->pointer = Lattimore_Hiding;
            } else if (time_info.hours < 11) {
                strcpy(ch->player.long_descr, lattimore_descs[4]);
                if (ch->in_room != Barracks) {
                    char_from_room(ch);
                    char_to_room(ch, Barracks);
                }
                mem->pointer = Lattimore_Sleeping;
            } else if (time_info.hours < 16 ||
                       (time_info.hours > 17 && time_info.hours < 22)) {
                strcpy(ch->player.long_descr, lattimore_descs[0]);
                if (ch->in_room != Barracks) {
                    char_from_room(ch);
                    char_to_room(ch, Barracks);
                }
                mem->pointer = Lattimore_Lockers;
            } else if (time_info.hours < 19) {
                strcpy(ch->player.long_descr, lattimore_descs[1]);
                mem->pointer = Lattimore_FoodRun;
            }
            return (FALSE);
            break;

        case Lattimore_Lockers:
            if (time_info.hours == 17) {
                strcpy(ch->player.long_descr, lattimore_descs[1]);
                mem->pointer = Lattimore_FoodRun;
            } else if (time_info.hours > 21) {
                act("$n cocks his head, as if listening.",
                    FALSE, ch, 0, 0, TO_ROOM);
                act("$n looks frightened, and dives under the nearest bunk.",
                    FALSE, ch, 0, 0, TO_ROOM);
                strcpy(ch->player.long_descr, lattimore_descs[3]);
                mem->pointer = Lattimore_Hiding;
            }
            return (FALSE);
            break;

        case Lattimore_FoodRun:
            if (ch->in_room != Kitchen) {
                dir = choose_exit_global(ch->in_room, Kitchen, 100);
                if (dir < 0) {
                    act("$n says 'Man, am I lost!'", FALSE, ch, 0, 0, TO_ROOM);
                    dir = choose_exit_global(ch->in_room, Barracks, 100);
                    if (dir < 0) {
                        char_from_room(ch);
                        char_to_room(ch, Barracks);
                    }
                    return (FALSE);
                } else
                    go_direction(ch, dir);
            } else {
                act("$n gets a spoon off the counter, and ladles himself some"
                    " stew.", FALSE, ch, 0, 0, TO_ROOM);
                strcpy(ch->player.long_descr, lattimore_descs[2]);
                mem->pointer = Lattimore_Eating;
            }
            return (FALSE);
            break;

        case Lattimore_Eating:
            if (time_info.hours > 18) {
                act("$n rubs his stomach and smiles happily.",
                    FALSE, ch, 0, 0, TO_ROOM);
                strcpy(ch->player.long_descr, lattimore_descs[1]);
                mem->pointer = Lattimore_GoHome;
            } else if (!number(0, 2)) {
                act("$n gets some bread from the oven to go with his stew.",
                    FALSE, ch, 0, 0, TO_ROOM);
                act("$n dips the bread in the stew and eats it.",
                    FALSE, ch, 0, 0, TO_ROOM);
            }
            return (FALSE);
            break;

        case Lattimore_GoHome:
            if (ch->in_room != Barracks) {
                dir = choose_exit_global(ch->in_room, Barracks, 100);
                if (dir < 0) {
                    act("$n says 'Man, am I lost!'", FALSE, ch, 0, 0, TO_ROOM);
                    dir = choose_exit_global(ch->in_room, Kitchen, 100);
                    if (dir < 0) {
                        char_from_room(ch);
                        char_to_room(ch, Barracks);
                    }
                    return (FALSE);
                } else
                    go_direction(ch, dir);
            } else {
                act("$n pulls out a crowbar and tries to open another locker.",
                    FALSE, ch, 0, 0, TO_ROOM);
                strcpy(ch->player.long_descr, lattimore_descs[0]);
                mem->pointer = Lattimore_Lockers;
            }
            return (FALSE);
            break;

        case Lattimore_Hiding:
            if (time_info.hours > 5 && time_info.hours < 22) {
                strcpy(ch->player.long_descr, lattimore_descs[4]);
                mem->pointer = Lattimore_Sleeping;
            }
            return (FALSE);
            break;

        case Lattimore_Sleeping:
            if (time_info.hours > 11) {
                act("$n awakens, rises and stretches with a yawn.",
                    FALSE, ch, 0, 0, TO_ROOM);
                act("$n pulls out a crowbar and tries to open another locker.",
                    FALSE, ch, 0, 0, TO_ROOM);
                strcpy(ch->player.long_descr, lattimore_descs[0]);
                mem->pointer = Lattimore_Lockers;
            }
            return (FALSE);
            break;

        case Lattimore_Run:
            if (ch->in_room != Storeroom && ch->in_room != Trap) {
                if (ch->in_room == EarthQ) {
                    return (FALSE);
                }
                dir = choose_exit_global(ch->in_room, Storeroom, 100);
                if (dir < 0) {
                    act("$n says 'Man, am I lost!'", FALSE, ch, 0, 0, TO_ROOM);
                    dir = choose_exit_global(ch->in_room, Kitchen, 100);
                    if (dir < 0) {
                        char_from_room(ch);
                        char_to_room(ch, Barracks);
                    }
                    return (FALSE);
                } else
                    go_direction(ch, dir);
            } else if (ch->in_room == Trap) {
                if (!IS_AFFECTED(ch, AFF_FLYING)) {
                    /*
                     * Get him up off the floor
                     */
                    act("$n grins evilly, and quickly stands on a barrel.",
                        FALSE, ch, 0, 0, TO_ROOM);
                    SET_BIT(ch->specials.affected_by, AFF_FLYING);
                    strcpy(ch->player.long_descr, lattimore_descs[5]);
                    mem->index = 0;
                } else {
                    ++mem->index;
                }
                /*
                 * Wait a while, then go home
                 */
                if (mem->index == 50) {
                    go_direction(ch, 1);
                    mem->pointer = Lattimore_GoHome;
                    REMOVE_BIT(ch->specials.affected_by, AFF_FLYING);
                    strcpy(ch->player.long_descr, lattimore_descs[1]);
                }
            }
            return (FALSE);
            break;

        case Lattimore_Item:
            if (ch->in_room != Conf) {
                dir = choose_exit_global(ch->in_room, Conf, 100);
                if (dir < 0) {
                    act("$n says 'Man, am I lost!'", FALSE, ch, 0, 0, TO_ROOM);
                    dir = choose_exit_global(ch->in_room, Barracks, 100);
                    if (dir < 0) {
                        char_from_room(ch);
                        char_to_room(ch, Barracks);
                    }
                    return (FALSE);
                } else
                    go_direction(ch, dir);
            } else {
                for (t = real_roomp(ch->in_room)->people; t;
                     t = t->next_in_room) {
                    if (!IS_NPC(t) && CAN_SEE(ch, t) &&
                        !(strcmp(mem->names[mem->index], GET_NAME(t)))) {
                        act("$n crawls under the large table.",
                            FALSE, ch, 0, 0, TO_ROOM);
                        obj = read_object(PostKey, VIRTUAL);
                        if ((IS_CARRYING_N(t) + 1) < CAN_CARRY_N(t)) {
                            act("$N emerges with $p, and gives it to you.",
                                FALSE, t, obj, ch, TO_CHAR);
                            act("$n emerges with $p, and gives it to $N.",
                                FALSE, ch, obj, t, TO_ROOM);
                            obj_to_char(obj, t);
                        } else {
                            act("$n emerges with $p, and drops it for $N.",
                                FALSE, ch, obj, t, TO_ROOM);
                            obj_to_room(obj, ch->in_room);
                        }
                    }
                }

                /*
                 * Dude's not here - oh well, go home.
                 */
                /* Duty discharged */
                mem->status[mem->index] = 0;
                mem->pointer = Lattimore_GoHome;
                return (FALSE);
            }
            break;
        default:
            mem->pointer = Lattimore_Initialize;
            return (FALSE);
            break;

        }
    } else if (cmd == 72) {
        arg = one_argument(arg, obj_name);
        if (!*obj_name ||
            !(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying))) {
            return (FALSE);
        }
        only_argument(arg, player_name);
        if (!*player_name || !(latt = get_char_room_vis(ch, player_name))) {
            return (FALSE);
        }

        /*
         * the target is Lattimore
         */
        Lattimore = lattimore;
        if (mob_index[latt->nr].func == Lattimore) {
            if (!latt->act_ptr) {
                ch->act_ptr = (struct memory *) malloc(sizeof(struct memory));
                mem = ch->act_ptr;
                mem->pointer = mem->c = mem->index = 0;
            } else {
                mem = (void *) latt->act_ptr;
            }
            act("You give $p to $N.", TRUE, ch, obj, latt, TO_CHAR);
            act("$n gives $p to $N.", TRUE, ch, obj, latt, TO_ROOM);

            switch (obj->obj_flags.type_flag) {

            case ITEM_FOOD:
                if (obj->obj_flags.value[3]) {
                    act("$n sniffs $p, then discards it with disgust.",
                        TRUE, latt, obj, 0, TO_ROOM);
                    obj_from_char(obj);
                    obj_to_room(obj, ch->in_room);
                    if (!IS_MOB(ch) && CAN_SEE(latt, ch)) {
                        mem->index = affect_status(mem, latt, ch, -5);
                    } else {
                        return (TRUE);
                    }
                } else {
                    act("$n takes $p and hungrily wolfs it down.",
                        TRUE, latt, obj, 0, TO_ROOM);
                    extract_obj(obj);
                    if (!IS_MOB(ch) && CAN_SEE(latt, ch)) {
                        mem->index = affect_status(mem, latt, ch, 4);
                    } else {
                        return (TRUE);
                    }
                }
                break;
            case ITEM_KEY:
                /*
                 * What he really wants
                 */
                if (obj_index[obj->item_number].virtual == CrowBar) {
                    act("$n takes $p and jumps up and down in joy.",
                        TRUE, latt, obj, 0, TO_ROOM);
                    obj_from_char(obj);
                    if (!ch->equipment[HOLD]) {
                        equip_char(ch, obj, HOLD);
                    }
                    if (!IS_MOB(ch) && CAN_SEE(latt, ch)) {
                        mem->index = affect_status(mem, latt, ch, 20);
                    } else {
                        return (TRUE);
                    }
                }
                break;
            default:
                /*
                 * Any other types of items
                 */
                act("$n looks at $p curiously.", TRUE, latt, obj, 0,
                    TO_ROOM);
                if (!IS_MOB(ch) && CAN_SEE(latt, ch)) {
                    mem->index = affect_status(mem, latt, ch, 1);
                } else {
                    return (TRUE);
                }
                break;
            }

            /*
             * They gave something to him, and the status was affected,
             * now we set the pointer according to the status value
             */
            if (mem->status[mem->index] < 0) {
                strcpy(latt->player.long_descr, lattimore_descs[6]);
                mem->pointer = Lattimore_Run;
            } else if (mem->status[mem->index] > 19) {
                strcpy(latt->player.long_descr, lattimore_descs[6]);
                mem->pointer = Lattimore_Item;
            }
            return (TRUE);
        }
    }
    return (FALSE);
}

/*
 * Returns the index to the dude who did it
 */

int affect_status(struct memory *mem, struct char_data *ch,
                  struct char_data *t, int aff_status)
{
    int             i;

    if (mem->c) {
        for (i = 0; i < mem->c; ++i) {
            if (!(strcmp(GET_NAME(t), mem->names[i]))) {
                mem->status[i] += aff_status;
                return (i);
            }
        }

        mem->names = (char **) realloc(mem->names, (sizeof(long) * mem->c));
        mem->status = (int *) realloc(mem->status, (sizeof(long) * mem->c));
    } else {
        mem->names = (char **) malloc(sizeof(long));
        mem->status = (int *) malloc(sizeof(long));
    }

    mem->names[mem->c] = (char *) malloc(strlen(GET_NAME(t) + 2));
    strcpy(mem->names[mem->c], GET_NAME(t));
    mem->status[mem->c] = (int) malloc(sizeof(int));
    mem->status[mem->c] = aff_status;
    ++mem->c;
    return (mem->c - 1);
}

int coldcaster(struct char_data *ch, int cmd, char *arg,
               struct char_data *mob, int type)
{
    struct char_data *vict;
    byte            lspell;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    /*
     * Find a dude to to evil things upon !
     */

    vict = FindVictim(ch);

    if (!vict) {
        vict = ch->specials.fighting;
    }
    if (!vict) {
        return (FALSE);
    }
    lspell = number(0, 9);

    switch (lspell) {
    case 0:
    case 1:
    case 2:
    case 3:
        act("$N touches you!", 1, vict, 0, ch, TO_CHAR);
        cast_chill_touch(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 4:
    case 5:
    case 6:
        cast_cone_of_cold(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    case 7:
    case 8:
    case 9:
        cast_ice_storm(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        break;
    }

    return (TRUE);
}

int trapper(struct char_data *ch, int cmd, char *arg,
            struct char_data *mob, int type)
{
    struct char_data *tch;
    char            buf[256];

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    /*
     * Okay, the idea is this: If the PC or NPC in this room isn't flying,
     * it is walking on the trapper. Doesn't matter if it's sneaking, or
     * invisible, or whatever. The trapper will attack both PCs and NPCs,
     * so don't have a lot of wandering NPCs around it.
     */

    if (!ch->specials.fighting) {
        for (tch = real_roomp(ch->in_room)->people; tch;
             tch = tch->next_in_room) {
            if (ch != tch && !IS_IMMORTAL(tch) &&
                !IS_AFFECTED(tch, AFF_FLYING)) {
                set_fighting(ch, tch);
                return (TRUE);
            }
        }
        /*
         * Nobody here
         */
        return (FALSE);
    } else {
        if (GetMaxLevel(ch->specials.fighting) > MAX_MORT) {
            return (FALSE);
        }

        /*
         * Equipment must save against crush - will fail 25% of the time
         */
        DamageStuff(ch->specials.fighting, TYPE_CRUSH, number(0, 7));

        /*
         * Make the poor sucker save against paralzyation, or suffocate
         */
        if (saves_spell(ch->specials.fighting, SAVING_PARA)) {
            act("You can hardly breathe, $N is suffocating you!",
                FALSE, ch->specials.fighting, 0, ch, TO_CHAR);
            act("$N is stifling $n, who will suffocate soon!",
                FALSE, ch->specials.fighting, 0, ch, TO_ROOM);
            return (FALSE);
        } else {
            act("You gasp for air inside $N!",
                FALSE, ch->specials.fighting, 0, ch, TO_CHAR);
            act("$N stifles you. You asphyxiate and die!",
                FALSE, ch->specials.fighting, 0, ch, TO_CHAR);
            act("$n has suffocated inside $N!",
                FALSE, ch->specials.fighting, 0, ch, TO_ROOM);
            act("$n is dead!", FALSE, ch->specials.fighting, 0, ch, TO_ROOM);
            sprintf(buf, "%s has suffocated to death.",
                    GET_NAME(ch->specials.fighting));
            Log(buf);

            die(ch->specials.fighting, '\0');
            ch->specials.fighting = 0x0;
            return (TRUE);
        }
    }
}

int trogcook(struct char_data *ch, int cmd, char *arg,
             struct char_data *mob, int type)
{
    struct char_data *tch;
    struct obj_data *corpse;
    char            buf[MAX_INPUT_LENGTH];

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        if (GET_POS(ch) != POSITION_FIGHTING) {
            StandUp(ch);
        }
        return (FALSE);
    }

    for (tch = real_roomp(ch->in_room)->people; tch;
         tch = tch->next_in_room) {
        if (IS_NPC(tch) && IsAnimal(tch) && CAN_SEE(ch, tch)) {
            if (!check_soundproof(ch)) {
                act("$n cackles 'Something else for the pot!'", FALSE, ch,
                    0, 0, TO_ROOM);
            }
            hit(ch, tch, TYPE_UNDEFINED);
            return (TRUE);
        }
    }

    corpse = get_obj_in_list_vis(ch, "corpse",
                                 real_roomp(ch->in_room)->contents);

    if (corpse) {
        do_get(ch, "corpse", -1);
        act("$n cackles 'Into the soup with it!'", FALSE, ch, 0, 0, TO_ROOM);
        sprintf(buf, "put corpse pot");
        command_interpreter(ch, buf);
        return (TRUE);
    }
    return( FALSE );
}


#define DEITY 21124
#define DEITY_NAME "golgar"

int shaman(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
           int type)
{
    struct char_data *god,
                   *tch;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        if (!number(0, 3)) {
            for (tch = real_roomp(ch->in_room)->people; tch;
                 tch = tch->next_in_room) {
                if (!IS_NPC(tch) && GetMaxLevel(tch) > 20 && CAN_SEE(ch, tch)) {
                    if (!(god = get_char_room_vis(ch, DEITY_NAME))) {
                        act("$n screams 'Golgar, I summon thee to aid thy "
                            "servants!'", FALSE, ch, 0, 0, TO_ROOM);

                        if (!number(0, 8)) {
                            act("There is a blinding flash of light!",
                                FALSE, ch, 0, 0, TO_ROOM);
                            god = read_mobile(DEITY, VIRTUAL);
                            char_to_room(god, ch->in_room);
                        }
                    } else if (!number(0, 2)) {
                        act("$n shouts 'Now you will die!'", FALSE, ch, 0,
                            0, TO_ROOM);
                    }
                }
            }
        } else {
            return (cleric(ch, cmd, arg, mob, type));
        }
    }
    return( FALSE );
}

#define SHAMAN_NAME "shaman"

int golgar(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
           int type)
{
    struct char_data *shaman,
                   *tch;

    if (cmd) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        return (magic_user(ch, cmd, arg, mob, type));
    }

    if (!(shaman = get_char_room_vis(ch, SHAMAN_NAME))) {
        for (tch = real_roomp(ch->in_room)->people; tch;
             tch = tch->next_in_room) {
            if (IS_NPC(tch) && GET_RACE(tch) == RACE_TROGMAN &&
                tch->specials.fighting && !IS_NPC(tch->specials.fighting)) {
                act("$n growls 'Death to those attacking my people!'",
                    FALSE, ch, 0, 0, TO_ROOM);
                hit(ch, tch->specials.fighting, TYPE_UNDEFINED);
                return (FALSE);
            }
        }

        if (number(0, 5) == 0) {
            act("$n slowly fades into ethereal emptiness.",
                FALSE, ch, 0, 0, TO_ROOM);
            extract_char(ch);
        }
    } else {
        if (!shaman->specials.fighting) {
            act("$n growls 'How dare you summon me!'",
                FALSE, ch, 0, 0, TO_ROOM);
            hit(ch, shaman, TYPE_UNDEFINED);
            return (FALSE);
        } else {
            act("$n screams 'You dare touch my holy messenger!? DIE!'",
                FALSE, ch, 0, 0, TO_ROOM);
            hit(ch, shaman->specials.fighting, TYPE_UNDEFINED);
            return (FALSE);
        }
    }
    return( FALSE );
}

int troguard(struct char_data *ch, int cmd, char *arg,
             struct char_data *mob, int type)
{
    struct char_data *tch,
                   *good;
    int             max_good;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (ch->specials.fighting) {
        if (GET_POS(ch) == POSITION_FIGHTING) {
            FighterMove(ch);
        } else {
            StandUp(ch);
        }

        if (!check_soundproof(ch)) {
            act("$n shouts 'The enemy is upon us! Help me, my brothers!'",
                TRUE, ch, 0, 0, TO_ROOM);
            if (ch->specials.fighting) {
                CallForGuard(ch, ch->specials.fighting, 3, TROGCAVES);
            }
            return (TRUE);
        }
    }

    max_good = -1001;
    good = 0;

    for (tch = real_roomp(ch->in_room)->people; tch;
         tch = tch->next_in_room) {
        if (GET_ALIGNMENT(tch) > max_good && !IS_IMMORTAL(tch) &&
            GET_RACE(tch) != RACE_TROGMAN && GET_RACE(tch) != RACE_ARACHNID) {
            max_good = GET_ALIGNMENT(tch);
            good = tch;
        }
    }

    if (check_peaceful(ch, "")) {
        return FALSE;
    }
    if (good) {
        if (!check_soundproof(ch)) {
            act("$n screams 'Die invading scum! Take that!'",
                FALSE, ch, 0, 0, TO_ROOM);
        }
        hit(ch, good, TYPE_UNDEFINED);
        return (TRUE);
    }

    return (FALSE);
}

#define START_ROOM      21276
#define END_ROOM        21333
#define GhostSoldier    21138
#define GhostLieutenant 21139
#define Identifier      "gds"

int keystone(struct char_data *ch, int cmd, char *arg,
             struct char_data *mob, int type)
{
    /*
     * Must be a unique identifier for this mob type, or we lose
     */

    struct char_data *ghost,
                   *t,
                   *master;
    int             i;

    if (cmd || !AWAKE(ch)) {
        return (FALSE);
    }
    if (time_info.hours == 22 &&
        !(ghost = get_char_vis_world(ch, Identifier, 0))) {
        act("$n cries 'Awaken my soldiers! Our time is nigh!'",
            FALSE, ch, 0, 0, TO_ROOM);
        act("You suddenly feel very, very afraid.",
            FALSE, ch, 0, 0, TO_ROOM);

        for (i = START_ROOM; i < END_ROOM; ++i) {
            if (number(0, 2) == 0) {
                ghost = read_mobile(GhostSoldier, VIRTUAL);
                char_to_room(ghost, i);
            } else if (number(0, 7) == 0) {
                ghost = read_mobile(GhostLieutenant, VIRTUAL);
                char_to_room(ghost, i);
            }
        }

        for (t = character_list; t; t = t->next) {
            if (real_roomp(ch->in_room)->zone == real_roomp(t->in_room)->zone) {
                act("You hear a strange cry that fills your soul with fear!",
                    FALSE, t, 0, 0, TO_CHAR);
            }
        }
    }

    if (ch->specials.fighting) {
        if (IS_NPC(ch->specials.fighting) &&
            !IS_SET((ch->specials.fighting)->specials.act, ACT_POLYSELF) &&
            (master = (ch->specials.fighting)->master) && CAN_SEE(ch, master)) {

            stop_fighting(ch);
            hit(ch, master, TYPE_UNDEFINED);
        }

        if (GET_POS(ch) == POSITION_FIGHTING) {
            FighterMove(ch);
        } else {
            StandUp(ch);
        }
        CallForGuard(ch, ch->specials.fighting, 3, OUTPOST);
    }

    return (FALSE);
}

int ghostsoldier(struct char_data *ch, int cmd, char *arg,
                 struct char_data *mob, int type)
{
    struct char_data *tch,
                   *good,
                   *master;
    int             max_good;
    int             (*gs) (),
                    (*gc) ();

    gs = ghostsoldier;
    gc = keystone;

    if (cmd) {
        return (FALSE);
    }
    if (time_info.hours > 4 && time_info.hours < 22) {
        act("$n slowly fades out of existence.", FALSE, ch, 0, 0, TO_ROOM);
        extract_char(ch);
        return (TRUE);
    }

    max_good = -1001;
    good = 0;

    for (tch = real_roomp(ch->in_room)->people; tch;
         tch = tch->next_in_room) {
        if (!(mob_index[tch->nr].func == gs) &&
            !(mob_index[tch->nr].func == gc) &&
            GET_ALIGNMENT(tch) > max_good && !IS_IMMORTAL(tch) &&
            GET_RACE(tch) >= 4) {
            max_good = GET_ALIGNMENT(tch);
            good = tch;
        }
    }
    /*
     * What is a ghost Soldier doing in a peaceful room?
     */
    if (check_peaceful(ch, "")) {
        return FALSE;
    }
    if (good) {
        if (!check_soundproof(ch)) {
            act("$N attacks you with an unholy scream!", FALSE, good, 0,
                ch, TO_CHAR);
        }
        hit(ch, good, TYPE_UNDEFINED);
        return (TRUE);
    }

    if (ch->specials.fighting) {
        if (IS_NPC(ch->specials.fighting) &&
            !IS_SET((ch->specials.fighting)->specials.act, ACT_POLYSELF) &&
            (master = (ch->specials.fighting)->master) && CAN_SEE(ch, master)) {
            stop_fighting(ch);
            hit(ch, master, TYPE_UNDEFINED);
        }

        if (GET_POS(ch) == POSITION_FIGHTING) {
            FighterMove(ch);
        } else {
            StandUp(ch);
        }
        CallForGuard(ch, ch->specials.fighting, 3, OUTPOST);
    }
    return (FALSE);
}

char           *quest_one[] = {
    "The second artifact you must find is the ring of Tlanic.",
    "Tlanic was an elven warrior who left Rhyodin five years after",
    "Lorces; he also was given an artifact to aid him.",
    "He went to find out what happened to Lorces, his friend, and to",
    "find a way to the north if he could.",
    "He also failed. Return the ring to me for further instructions."
};

char           *quest_two[] = {
    "When Tlanic had been gone for many moons; his brother Evistar",
    "went to find him.",
    "Evistar, unlike his brother, was not a great warrior, but he was",
    "the finest tracker among our people.",
    "Given to him as an aid to his travels, was a neverempty chalice.",
    "Bring this magical cup to me, if you wish to enter the kingdom",
    "of the Rhyodin."
};

char           *quest_three[] = {
    "When Evistar did not return, ages passed before another left.",
    "A mighty wizard was the next to try. His name was C*zarnak.",
    "It is feared that he was lost in the deep caves, like the others.",
    "He wore an enchanted circlet on his brow.",
    "Find it and bring it to me, and I will tell you more."
};

char           *necklace[] = {
    "You have brought me all the items lost by those who sought the",
    "path from the kingdom to the outside world.",
    "Furthermore, you have found the way through the mountains",
    "yourself, proving your ability to track and map.",
    "You are now worthy to be an ambassador to my kingdom.",
    "Take this necklace, and never part from it!",
    "Give it to the gatekeeper, and he will let you pass.",
};

char           *nonecklace[] = {
    "You have brought me all the items lost by those who sought the",
    "path from the kingdom to the outside world.",
    "Furthermore, you have found the way through the mountains",
    "yourself, proving your ability to track and map.",
    "You are now worthy to be an ambassador to my kingdom.",
    "Your final quest is to obtain the Necklace of Wisdom.",
    "The gatekeeper will recognize it, and let you pass.",
};

char           *quest_intro[] = {
    "My name is Valik, and I am the Lorekeeper of the Rhyodin.",
    "Rhyodin is kingdom southeast of the Great Eastern Desert.",
    "To enter the kingdom of Rhyodin, you must first pass this test.",
    "When the only route to the kingdom collapsed, commerce with",
    "the outside world was cutoff.",
    "Lorces was the first to try to find a path back to the outside,",
    "and for his bravery to him a shield was given.",
    "The first step towards entry to the kingdom of Rhyodin is to",
    "return to me the shield of Lorces.",
};


#define Valik_Wandering   0
#define Valik_Meditating  1
#define Valik_Qone        2
#define Valik_Qtwo        3
#define Valik_Qthree      4
#define Valik_Qfour       5
#define Shield            21113
#define Ring              21120
#define Chalice           21121
#define Circlet           21117
#define Necklace          21122
#define Med_Chambers      21324

int Valik(struct char_data *ch, int cmd, char *arg, struct char_data *mob,
          int type)
{
    char            obj_name[80],
                    vict_name[80],
                    buf[MAX_INPUT_LENGTH];
    int             i;
    struct char_data *vict,
                   *master;
    struct obj_data *obj = NULL;
    int             (*valik) ();
    bool            gave_this_click = FALSE;
    short           quest_lines[4];
    short           valik_dests[9];

    if ((cmd && cmd != 72 && cmd != 86) || !AWAKE(ch)) {
        return (FALSE);
    }

    quest_lines[0] = 6;
    quest_lines[1] = 7;
    quest_lines[2] = 5;
    quest_lines[3] = 7;

    valik_dests[0] = 104;
    valik_dests[1] = 1638;
    valik_dests[2] = 7902;
    valik_dests[3] = 13551;
    valik_dests[4] = 16764;
    valik_dests[5] = 17330;
    valik_dests[6] = 19244;
    valik_dests[7] = 21325;
    valik_dests[8] = 25230;

    if (!cmd) {
        if (ch->specials.fighting) {
            if (IS_NPC(ch->specials.fighting) &&
                !IS_SET((ch->specials.fighting)->specials.act, ACT_POLYSELF) &&
                (master = (ch->specials.fighting)->master) &&
                CAN_SEE(ch, master)) {

                stop_fighting(ch);
                hit(ch, master, TYPE_UNDEFINED);
            }
            return (magic_user(ch, cmd, arg, mob, type));
        }
    }

    vict = FindMobInRoomWithFunction(ch->in_room, Valik);
    assert(vict != 0);

    valik = Valik;

    switch (vict->generic) {
    case Valik_Wandering:
    case Valik_Qone:
    case Valik_Qtwo:
    case Valik_Qthree:
        if (cmd == 72) {
            /*
             * give
             */
            arg = one_argument(arg, obj_name);
            if (!*obj_name ||
                !(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying)))
                return (FALSE);

            only_argument(arg, vict_name);
            if (!*vict_name || !(vict = get_char_room_vis(ch, vict_name)))
                return (FALSE);

            /*
             * the target is valik
             */
            if (mob_index[vict->nr].func == valik) {
                act("You give $p to $N.", TRUE, ch, obj, vict, TO_CHAR);
                act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_ROOM);
            } else {
                return (FALSE);
            }
            gave_this_click = TRUE;
        } else if (cmd == 86) {
            /*
             * ask
             */
            arg = one_argument(arg, vict_name);
            if (!*vict_name || !(vict = get_char_room_vis(ch, vict_name))) {
                return (FALSE);
            }
            if (mob_index[vict->nr].func != valik) {
                return (FALSE);
            } else {
                if (!(strcmp(arg, " What is the quest of the Rhyodin?"))) {
                    for (i = 0; i < 9; ++i) {
                        sprintf(buf, "%s %s", GET_NAME(ch), quest_intro[i]);
                        do_tell(vict, buf, 19);
                    }
                }
                return (TRUE);
            }
        }
        break;
    case Valik_Meditating:
        if (time_info.hours < 22 && time_info.hours > 5) {
            do_stand(ch, "", -1);
            act("$n says 'Perhaps today will be different.'", FALSE, ch, 0,
                0, TO_ROOM);
            act("$n slowly fades out of existence.", FALSE, ch, 0, 0, TO_ROOM);
            char_from_room(ch);
            char_to_room(ch, valik_dests[number(0, 8)]);

            act("The world warps, dissolves, and reforms.", FALSE, ch, 0,
                0, TO_ROOM);
            ch->generic = Valik_Wandering;
            return (FALSE);
        } else {
            for (vict = real_roomp(ch->in_room)->people; vict;
                 vict = vict->next_in_room) {
                if (!IS_NPC(vict) && GetMaxLevel(vict) < LOW_IMMORTAL &&
                    !number(0, 3)) {
                    act("$n snaps out of his meditation.", FALSE, ch, 0, 0,
                        TO_ROOM);
                    do_stand(ch, "", -1);
                    hit(ch, vict, TYPE_UNDEFINED);
                    return (FALSE);
                }
            }
            return (FALSE);
        }
        break;
    default:
        ch->generic = Valik_Wandering;
        return (FALSE);
    }

    /*
     * There are four valid objects
     */
    switch (vict->generic) {
    case Valik_Wandering:
        if (gave_this_click) {
            /*
             * Take it, in either case
             */
            obj_from_char(obj);
            obj_to_char(obj, vict);
            if (obj_index[obj->item_number].virtual == Shield) {
                if (!check_soundproof(ch)) {
                    act("$N says 'The Shield of Lorces!'", FALSE, ch, 0,
                        vict, TO_CHAR);
                    act("$N says 'You may now undertake the first quest.'",
                        FALSE, ch, 0, vict, TO_CHAR);
                }
                vict->generic = Valik_Qone;
            } else {
                act("$N takes the $p and bows in thanks.'",
                    FALSE, ch, obj, vict, TO_CHAR);
                act("$N takes the $p from $n and bows in thanks.'",
                    FALSE, ch, obj, vict, TO_ROOM);
                return (TRUE);
            }
        } else if (cmd) {
            return (FALSE);
        } else {
            if (time_info.hours > 21) {
                if (!check_soundproof(ch)) {
                    act("$n says 'It is time to meditate.'", FALSE, ch, 0,
                        0, TO_ROOM);
                }
                act("$n disappears in a flash of light!", FALSE, ch, 0, 0,
                    TO_ROOM);
                char_from_room(ch);
                char_to_room(ch, Med_Chambers);

                act("Reality warps and spins around you!", FALSE, ch, 0, 0,
                    TO_ROOM);
                sprintf(buf, "close mahogany");
                command_interpreter(ch, buf);
                do_rest(ch, "", -1);
                ch->generic = Valik_Meditating;
                return (FALSE);
            }
            return (magic_user(ch, cmd, arg, mob, type));
        }
        break;
    case Valik_Qone:
        if (gave_this_click) {
            if (obj_index[obj->item_number].virtual == Ring) {
                if (!check_soundproof(ch)) {
                    act("$N says 'You have brought me the ring of Tlanic.'",
                        FALSE, ch, 0, vict, TO_CHAR);
                    act("$N says 'You may now undertake the second quest.'",
                        FALSE, ch, 0, vict, TO_CHAR);
                }
                obj_from_char(obj);
                obj_to_char(obj, vict);
                vict->generic = Valik_Qtwo;
            } else {
                act("$N shakes his head - it is the wrong item.",
                    FALSE, ch, 0, vict, TO_CHAR);
                act("$N gives $p back to you.", TRUE, ch, obj, vict,
                    TO_CHAR);
                act("$N gives $p to $n.", TRUE, ch, obj, vict, TO_ROOM);
                return (TRUE);
            }
        } else {
            return (FALSE);
        }
        break;
    case Valik_Qtwo:
        if (gave_this_click) {
            if (obj_index[obj->item_number].virtual == Chalice) {
                if (!check_soundproof(ch)) {
                    act("$N says 'You have brought me the chalice of Evistar.'",
                        FALSE, ch, 0, vict, TO_CHAR);
                    act("$N says 'You may now undertake the third quest.'",
                        FALSE, ch, 0, vict, TO_CHAR);
                }
                obj_from_char(obj);
                obj_to_char(obj, vict);
                vict->generic = Valik_Qthree;
            } else {
                act("$N shakes his head - it is the wrong item.'",
                    FALSE, ch, 0, 0, TO_CHAR);
                act("$N gives $p back to you.", TRUE, ch, obj, vict,
                    TO_CHAR);
                act("$N gives $p to $n.", TRUE, ch, obj, vict, TO_ROOM);
                return (TRUE);
            }
        } else {
            return (FALSE);
        }
        break;
    case Valik_Qthree:
        if (gave_this_click) {
            if (obj_index[obj->item_number].virtual == Circlet) {
                if (!check_soundproof(ch)) {
                    act("$N says 'You have brought me the circlet of "
                        "C*zarnak.'", FALSE, ch, 0, vict, TO_CHAR);
                    act("$N says 'You may now undertake the final quest.'",
                        FALSE, ch, 0, vict, TO_CHAR);
                }
                obj_from_char(obj);
                obj_to_char(obj, vict);
                vict->generic = Valik_Qfour;
            } else {
                act("$N says 'That is not the item I require.'",
                    FALSE, ch, 0, vict, TO_CHAR);
                act("$N gives $p back to you.", TRUE, ch, obj, vict, TO_CHAR);
                act("$N gives $p to $n.", TRUE, ch, obj, vict, TO_ROOM);
                return (TRUE);
            }
        } else {
            return (FALSE);
        }
        break;
    default:
        return (FALSE);
    }

    /*
     * The final switch, where we tell the player what the quests are
     */
    switch (vict->generic) {
    case Valik_Qone:
        for (i = 0; i < quest_lines[vict->generic - 2]; ++i)
            do_say(vict, quest_one[i], 0);
        break;
    case Valik_Qtwo:
        for (i = 0; i < quest_lines[vict->generic - 2]; ++i)
            do_say(vict, quest_two[i], 0);
        break;
    case Valik_Qthree:
        for (i = 0; i < quest_lines[vict->generic - 2]; ++i)
            do_say(vict, quest_three[i], 0);
        break;
    case Valik_Qfour:
        if (vict->equipment[WEAR_NECK_1] &&
            obj_index[vict->equipment[WEAR_NECK_1]->item_number].virtual ==
                Necklace) {

            for (i = 0; i < quest_lines[vict->generic - 2]; ++i) {
                do_say(vict, necklace[i], 0);
            }

            act("$N takes the Necklace of Wisdom and hands it to you.",
                FALSE, ch, 0, vict, TO_CHAR);
            act("$N takes the Necklace of Wisdom and hands it to $n.",
                FALSE, ch, 0, vict, TO_ROOM);
            obj_to_char(unequip_char(vict, WEAR_NECK_1), ch);
        } else {
            for (i = 0; i < quest_lines[vict->generic - 2]; ++i) {
                do_say(vict, nonecklace[i], 0);
            }
        }
        vict->generic = Valik_Wandering;
        break;
    default:
        Log("Ack! Foo! Heimdall screws up!");
        return (FALSE);
    }
    return (TRUE);
}

#define RHYODIN_FILE "rhyodin"
#define Necklace 21122

int guardian(struct char_data *ch, int cmd, char *arg,
             struct char_data *mob, int type)
{
    FILE           *pass;
    struct char_data *g,
                   *master;
    struct obj_data *obj;
    struct room_data *rp;
    int             j = 0;
    struct follow_type *fol;
    char            player_name[80],
                    obj_name[80],
                    name[15];
    int             (*guard) ();

    struct Names {
        char          **names;
        short           num_names;
    }              *gstruct;

    if ((cmd && !(cmd == 72 || cmd == 3)) || !AWAKE(ch)) {
        return (FALSE);
    }

    if (!cmd && ch->generic == -1) {
        return (FALSE);
    }
    if (!cmd && !ch->generic) {
        /*
         * Open the file, read the names into an array in the act pointer
         */
        if (!(pass = fopen(RHYODIN_FILE, "r"))) {
            Log("Rhyodin access file unreadable or non-existant");
            ch->generic = -1;
            return (FALSE);
        }

        ch->act_ptr = (struct Names *) malloc(sizeof(struct Names));
        gstruct = ch->act_ptr;
        gstruct->names = (char **) malloc(sizeof(char));
        gstruct->num_names = 0;

        while (fscanf(pass, " %s\n", name) == 1) {
            gstruct->names = (char **)
                realloc(gstruct->names, (++gstruct->num_names) * sizeof(char));
            gstruct->names[gstruct->num_names - 1] = (char *)
                malloc(15 * (sizeof(char)));
            strcpy(gstruct->names[gstruct->num_names - 1], name);
        }
    } else {
        gstruct = (void *) ch->act_ptr;
    }

    if (!cmd) {
        if (ch->specials.fighting) {
            if (IS_NPC(ch->specials.fighting) &&
                !IS_SET((ch->specials.fighting)->specials.act, ACT_POLYSELF) &&
                (master = (ch->specials.fighting)->master) &&
                CAN_SEE(ch, master)) {

                stop_fighting(ch);
                hit(ch, master, TYPE_UNDEFINED);
            }

            if (GET_POS(ch) == POSITION_FIGHTING) {
                FighterMove(ch);
            } else {
                StandUp(ch);
            }
        }
        return (FALSE);
    }

    if (cmd == 72) {
        arg = one_argument(arg, obj_name);
        if (!*obj_name ||
            !(obj = get_obj_in_list_vis(ch, obj_name, ch->carrying))) {
            return (FALSE);
        }
        only_argument(arg, player_name);
        if (!*player_name || !(g = get_char_room_vis(ch, player_name))) {
            return (FALSE);
        }
        guard = guardian;

        if (mob_index[g->nr].func == guard) {
            gstruct = (void *) g->act_ptr;

            act("You give $p to $N.", TRUE, ch, obj, g, TO_CHAR);
            act("$n gives $p to $N.", TRUE, ch, obj, g, TO_ROOM);

            if (obj_index[obj->item_number].virtual == Necklace) {
                if (!check_soundproof(ch)) {
                    act("$n takes $p, and unlocks the gate.",
                        FALSE, g, obj, 0, TO_ROOM);
                    act("$p pulses in his hand, and disappears.",
                        FALSE, g, obj, 0, TO_ROOM);
                    act("$N says 'You have proven youself worthy.'",
                        FALSE, ch, 0, g, TO_CHAR);
                    act("$N says 'You are now an ambassador from the north "
                        "to Rhyodin.'", FALSE, ch, 0, g, TO_CHAR);
                }

                /*
                 * Take it away
                 */
                obj_from_char(obj);
                extract_obj(obj);

                if (!IS_NPC(ch)) {
                    if (!(pass = fopen(RHYODIN_FILE, "a"))) {
                        Log("Couldn't open file for writing permanent Rhyodin "
                            "passlist.");
                        return (FALSE);
                    }

                    /*
                     * Go to the end of the file and write the character's
                     * name
                     */
                    fprintf(pass, " %s\n", GET_NAME(ch));
                    fclose(pass);
                }

                /*
                 * Okay, now take person and all followers in this room to
                 * next room
                 */
                act("$N opens the gate and guides you through.",
                    FALSE, ch, 0, g, TO_CHAR);
                rp = real_roomp(ch->in_room);

                char_from_room(ch);
                char_to_room(ch, rp->dir_option[2]->to_room);
                do_look(ch, "\0", 0);

                /*
                 * First level followers can tag along
                 */
                if (ch->followers) {
                    act("$N says 'If they're with you, they can enter as "
                        "well.'", FALSE, ch, 0, g, TO_CHAR);

                    for (fol = ch->followers; fol; fol = fol->next) {
                        if (fol->follower->specials.fighting) {
                            continue;
                        }
                        if (real_roomp(fol->follower->in_room) &&
                            EXIT(fol->follower, 2)->to_room != NOWHERE &&
                            GET_POS(fol->follower) >= POSITION_STANDING) {
                            char_from_room(fol->follower);
                            char_to_room(fol->follower,
                                         rp->dir_option[2]->to_room);
                            do_look(fol->follower, "\0", 0);
                        }
                    }
                }
                return (TRUE);
            } else {
                return (FALSE);
            }
        } else {
            return (FALSE);
        }
    } else if (cmd == 3 && !IS_NPC(ch)) {
        g = find_mobile_here_with_spec_proc(guardian, ch->in_room);
        gstruct = (void *) g->act_ptr;
        j = 0;

        /*
         * Trying to move south, check against namelist
         */
        while (j < gstruct->num_names) {
            if (!strcmp(gstruct->names[j], GET_NAME(ch))) {
                if (real_roomp(ch->in_room)
                    && (EXIT(ch, 2)->to_room != NOWHERE)) {
                    if (ch->specials.fighting) {
                        return (FALSE);
                    }
                    act("$N recognizes you, and escorts you through the gate.",
                        FALSE, ch, 0, g, TO_CHAR);
                    act("$N recognizes $n, and escorts them through the gate.",
                        FALSE, ch, 0, g, TO_ROOM);
                    rp = real_roomp(ch->in_room);
                    char_from_room(ch);
                    char_to_room(ch, rp->dir_option[2]->to_room);
                    do_look(ch, "\0", 0);

                    /*
                     * Follower stuff again
                     */
                    if (ch->followers) {
                        act("$N says 'If they're with you, they can enter as "
                            "well.'", FALSE, ch, 0, g, TO_CHAR);

                        for (fol = ch->followers; fol; fol = fol->next) {
                            if (fol->follower->specials.fighting) {
                                continue;
                            }
                            if (real_roomp(fol->follower->in_room) &&
                                EXIT(fol->follower, 2)->to_room != NOWHERE &&
                                GET_POS(fol->follower) >= POSITION_STANDING) {

                                char_from_room(fol->follower);
                                char_to_room(fol->follower,
                                             rp->dir_option[2]->to_room);
                                do_look(fol->follower, "\0", 0);
                            }
                        }
                    }
                    return (TRUE);
                } else {
                    return (FALSE);
                }
            }
            ++j;
        }
        return (FALSE);
    }
    return (FALSE);
}

int web_slinger(struct char_data *ch, int cmd, char *arg,
                struct char_data *mob, int type)
{
    struct char_data *vict;

    if (cmd || !AWAKE(ch) || IS_AFFECTED(ch, AFF_BLIND)) {
        return (FALSE);
    }
    if (GET_POS(ch) > POSITION_STUNNED && GET_POS(ch) < POSITION_FIGHTING) {
        StandUp(ch);
        return (TRUE);
    }

    /*
     * Find a dude to to evil things upon !
     */
    vict = FindVictim(ch);

    if (!vict) {
        vict = ch->specials.fighting;
    }
    if (!vict) {
        return (FALSE);
    }
    if (!number(0, 3)) {
        /*
         * Noop, for now
         */
        act("$n casts sticky webs upon you!", TRUE, ch, 0, 0, TO_ROOM);
        cast_web(GetMaxLevel(ch), ch, "", SPELL_TYPE_SPELL, vict, 0);
        return( TRUE );
    }
    return( FALSE );
}

/*
 * vim:ts=4:sw=4:ai:et:si:sts=4
 */