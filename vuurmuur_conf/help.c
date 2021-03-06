/***************************************************************************
 *   Copyright (C) 2003-2017 by Victor Julien                              *
 *   victor@vuurmuur.org                                                   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "main.h"

#define UTF8_TRUE   TRUE
#define UTF8_FALSE  FALSE

typedef struct
{
    char    newline;
    char    *word;
    size_t  line_num;
} helpword;

/* wide variant */
typedef struct
{
    char    newline;
    wchar_t *word;
    size_t  line_num;
} whelpword;

static void
free_helpword(void *ptr)
{
    helpword    *hw = NULL;

    if(!ptr)
        return;

    hw = (helpword *)ptr;
    if (hw->word != NULL)
        free(hw->word);
    free(hw);
}

/* parse a line from the helpfile */
int
read_helpline(const int debuglvl, struct vrmr_list *help_list, char *line)
{
    char        oneword[512] = "";
    size_t      i = 0;
    size_t      k = 0;
    helpword    *hw = NULL;

    for(i = 0, k = 0; i < StrMemLen(line); i++)
    {
        if(line[i] == ' ' || line[i] == '\n')
        {
            oneword[k] = '\0';
            k = 0;

            /* only add a word to the list if it really contains characters */
            if(StrLen(oneword) > 0)
            {
                /* get some mem for the word struct */
                hw = malloc(sizeof(helpword));
                vrmr_fatal_alloc("malloc", hw);
                hw->word = NULL;
                hw->newline = 0;
                hw->line_num = 0;

                hw->word = strdup(oneword);
                vrmr_fatal_alloc("strdup", hw->word);

                vrmr_fatal_if(vrmr_list_append(debuglvl, help_list, hw) == NULL);
            }

            /* the newline is a special word */
            if( (i == 0 && line[i] == '\n') ||

                (i > 0  && line[i] == '\n' &&
                (line[i-1] == '.' || line[i-1] == '?' ||
                 line[i-1] == '!' || line[i-1] == ':' ||
                (line[i-1] == ',' && i == (StrMemLen(line) - 1) ))))
            {
                /* get some mem for the word struct */
                hw = malloc(sizeof(helpword));
                vrmr_fatal_alloc("malloc", hw);
                hw->word = NULL;
                hw->newline = 1;
                hw->line_num = 0;

                vrmr_fatal_if(vrmr_list_append(debuglvl, help_list, hw) == NULL);
            }
        }
        else
        {
            oneword[k] = line[i];
            k++;
        }
    }

    return(0);
}

#ifdef USE_WIDEC
/* parse a line from the UTF-8 helpfile */
int
read_wide_helpline(const int debuglvl, struct vrmr_list *help_list, wchar_t *line)
{
    wchar_t     oneword[512] = L"";
    size_t      i = 0;
    int         k = 0;
    whelpword   *hw = NULL;

    for(i = 0, k = 0; i < wcslen(line); i++)
    {
        if(line[i] == L' ' || line[i] == L'\n')
        {
            oneword[k] = L'\0';
            k = 0;

            /* only add a word to the list if it really contains characters */
            if(wcslen(oneword) > 0)
            {
                hw = malloc(sizeof(whelpword));
                vrmr_fatal_alloc("malloc", hw);
                hw->word = NULL;
                hw->newline = 0;
                hw->line_num = 0;

                hw->word = calloc(wcslen(oneword) + 1, sizeof(wchar_t));
                vrmr_fatal_alloc("calloc", hw->word);
                wcsncpy(hw->word, oneword, wcslen(oneword) + 1);

                vrmr_fatal_if(vrmr_list_append(debuglvl, help_list, hw) == NULL);
            }

            /* the newline is a special word */
            if( (i == 0 && line[i] == L'\n') ||

                (i > 0  && line[i] == L'\n' &&
                (line[i-1] == L'.' || line[i-1] == L'?' ||
                 line[i-1] == L'!' || line[i-1] == L':' ||
                (line[i-1] == L',' && i == (wcslen(line) - 1) ))))
            {
                /* get some mem for the word struct */
                hw = malloc(sizeof(whelpword));
                vrmr_fatal_alloc("malloc", hw);
                hw->word = NULL;
                hw->newline = 1;
                hw->line_num = 0;

                vrmr_fatal_if(vrmr_list_append(debuglvl, help_list, hw) == NULL);
            }
        }
        else
        {
            oneword[k] = line[i];
            k++;
        }
    }

    return(0);
}
#endif /* USE_WIDEC */

static int
read_helpfile(const int debuglvl, struct vrmr_list *help_list, char *part)
{
    char    line[128] = "";
    FILE    *fp = NULL;
    char    inrange = 0;
    char    helpfile[256] = "";

    /* safety */
    vrmr_fatal_if_null(help_list);
    vrmr_fatal_if_null(part);

    /* setup the list */
    vrmr_list_setup(debuglvl, help_list, free_helpword);

    /* TRANSLATORS: translate this to you language code: so for
       'ru' use 'vuurmuur-ru.hlp', for 'pt_BR' use
       'vuurmuur-pt_BR.hlp'
     */
    snprintf(helpfile, sizeof(helpfile), "%s/%s",
            vccnf.helpfile_location,
            gettext("vuurmuur.hlp"));
    vrmr_sanitize_path(debuglvl, helpfile, sizeof(helpfile));

    /* open the file */
    fp = fopen(helpfile, "r");
    if (fp == NULL)
    {
        vrmr_debug(__FUNC__, "opening '%s' failed: %s, "
                "falling back to default.",
                helpfile, strerror(errno));

        /* language helpfile does not exist, try to fall back to default */
        snprintf(helpfile, sizeof(helpfile), "%s/vuurmuur.hlp",
                vccnf.helpfile_location);
        vrmr_sanitize_path(debuglvl, helpfile, sizeof(helpfile));

        if(!(fp = fopen(helpfile, "r")))
        {
            vrmr_error(-1, VR_ERR, "%s %s: %s",
                    STR_OPENING_FILE_FAILED,
                    helpfile, strerror(errno));
            return(-1);
        }
    }

    while(fgets(line, (int)sizeof(line), fp) != NULL)
    {
        if (inrange) {
            if(strcmp(line, ":[END]:\n") == 0)
            {
                /* implied inrange = 0; */
                break;
            }
        }

        if(inrange) {
            if (read_helpline(debuglvl, help_list, line) < 0) {
                fclose(fp);
                return(-1);
            }
        } else {
            if (strncmp(line, part, StrMemLen(part)) == 0)
                inrange = 1;
        }
    }
    fclose(fp);
    return(0);
}


#ifdef USE_WIDEC
int
read_wide_helpfile(const int debuglvl, struct vrmr_list *help_list, wchar_t *part)
{
    wchar_t line[128] = L"";
    FILE    *fp = NULL;
    char    inrange = 0;
    char    helpfile[256] = "";

    /* safety */
    vrmr_fatal_if_null(help_list);
    vrmr_fatal_if_null(part);

    /* setup the list */
    vrmr_list_setup(debuglvl, help_list, free_helpword);

    if (utf8_mode == 1) {
        /* TRANSLATORS: translate this to you language code: so for
          'ru' use 'vuurmuur-ru.UTF-8.hlp', for 'pt_BR' use
          'vuurmuur-pt_BR.UTF-8.hlp'
        */
        snprintf(helpfile, sizeof(helpfile), "%s/%s",
                vccnf.helpfile_location,
                gettext("vuurmuur.UTF-8.hlp"));
        vrmr_sanitize_path(debuglvl, helpfile, sizeof(helpfile));

        /* open the file */
        fp = fopen(helpfile, "r");
    }

    if (fp == NULL) {
        if (utf8_mode == 1)
            vrmr_debug(__FUNC__, "opening '%s' failed: "
                "%s, falling back to non UTF-8 language file.",
                helpfile, strerror(errno));

        /* UTF-8 language helpfile does not exist,
           try to fall back to default */

        /* TRANSLATORS: translate this to you language code: so for
           'ru' use 'vuurmuur-ru.hlp', for 'pt_BR' use
           'vuurmuur-pt_BR.hlp'
         */
        snprintf(helpfile, sizeof(helpfile), "%s/%s",
                vccnf.helpfile_location,
                gettext("vuurmuur.hlp"));
        vrmr_sanitize_path(debuglvl, helpfile, sizeof(helpfile));

        /* open the file */
        fp = fopen(helpfile, "r");
        if (fp == NULL) {
            vrmr_debug(__FUNC__, "opening '%s' failed: %s, "
                    "falling back to default.",
                    helpfile, strerror(errno));

            /* language helpfile does not exist, try to fall back to default */
            snprintf(helpfile, sizeof(helpfile), "%s/vuurmuur.hlp",
                    vccnf.helpfile_location);
            vrmr_sanitize_path(debuglvl, helpfile, sizeof(helpfile));

            if(!(fp = fopen(helpfile, "r"))) {
                vrmr_error(-1, VR_ERR, "%s %s: %s",
                        STR_OPENING_FILE_FAILED,
                        helpfile, strerror(errno));
                return(-1);
            }
        }
    }

    while(fgetws(line, wsizeof(line), fp) != NULL)
    {
        if(inrange) {
            if(wcscmp(line, L":[END]:\n") == 0) {
                /* implied inrange = 0; */
                break;
            }
        }

        if(inrange) {
            if (read_wide_helpline(debuglvl, help_list, line) < 0) {
                fclose(fp);
                return(-1);
            }
        } else {
            if(wcsncmp(line, part, wcslen(part)) == 0)
                inrange = 1;
        }
    }

    fclose(fp);
    return(0);
}
#endif /* USE_WIDEC */

static void
set_lines(const int debuglvl, struct vrmr_list *help_list, size_t width)
{
    size_t      line_width = 0,
                line_num = 1,
                words = 0;
    helpword    *hw = NULL,
                *next_hw = NULL;
    struct vrmr_list_node *d_node = NULL,
                *next_d_node = NULL;

    /* safety */
    vrmr_fatal_if_null(help_list);

    for(d_node = help_list->top; d_node; d_node = d_node->next, words++)
    {
        vrmr_fatal_if_null(d_node->data);
        hw = d_node->data;

        if(hw->word != NULL && !hw->newline) {
            /* get the next word */
            if((next_d_node = d_node->next))
            {
                vrmr_fatal_if_null(next_d_node->data);
                next_hw = next_d_node->data;

                /* middle in sentence, so we add a space to the word */
                if(next_hw->word != NULL && next_hw->newline == 0) {
                    /* don't add this on the current line */
                    if((StrLen(hw->word) + 1) >= (width - line_width))
                    {
                        /* set line_width to the size of this word */
                        line_width = StrLen(hw->word) + 1;
                        line_num++;
                        hw->line_num = line_num;
                    }
                    /* this word fits on the current line */
                    else
                    {
                        /* add this word to the line_width */
                        line_width = line_width + StrLen(hw->word) + 1;
                        hw->line_num = line_num;
                    }
                }
                /* end of sentence, so no trailing space */
                else if(next_hw->word == NULL && next_hw->newline == 1) {
                    /* don't add this on the current line */
                    if(StrLen(hw->word) >= (width - line_width))
                    {
                        /* set line_width to the size of this word */
                        line_width = StrLen(hw->word);
                        line_num++;
                        hw->line_num = line_num;
                    }
                    /* this word fits on the current line */
                    else
                    {
                        /* add this word to the line_width */
                        line_width = line_width + StrLen(hw->word);
                        hw->line_num = line_num;
                    }
                } else {
                    vrmr_fatal("undefined state");
                }
            }
            /* end of text, doc has no trailing new-line */
            else {
                /* don't add this on the current line */
                if(StrLen(hw->word) >= (width - line_width)) {
                    /* set line_width to the size of this word */
                    line_width = StrLen(hw->word);
                    line_num++;
                    hw->line_num = line_num;
                }
                /* this word fits on the current line */
                else
                {
                    /* add this word to the line_width */
                    line_width = line_width + StrLen(hw->word);
                    hw->line_num = line_num;
                }
            }
        } else if(hw->word == NULL && hw->newline) {
            hw->line_num = line_num;
            line_num++;
            line_width = 0;
        } else {
            vrmr_fatal("undefined state");
        }
    }
}

#ifdef USE_WIDEC
static void
set_wide_lines(const int debuglvl, struct vrmr_list *help_list, int width)
{
    int         line_width = 0,
                line_num = 1,
                words = 0;
    whelpword   *hw = NULL,
                *next_hw = NULL;
    struct vrmr_list_node *d_node = NULL,
                *next_d_node = NULL;

    /* safety */
    vrmr_fatal_if_null(help_list);

    for(d_node = help_list->top; d_node; d_node = d_node->next, words++)
    {
        vrmr_fatal_if_null(d_node->data);
        hw = d_node->data;

        if(hw->word != NULL && !hw->newline) {
            /* get the next word */
            if((next_d_node = d_node->next))
            {
                vrmr_fatal_if_null(next_d_node->data);
                next_hw = next_d_node->data;

                /* middle in sentence, so we add a space to the word */
                if(next_hw->word != NULL && next_hw->newline == 0) {
                    /* don't add this on the current line */
                    if((int)(wcslen(hw->word) + 1) >= (width - line_width)) {
                        /* set line_width to the size of this word */
                        line_width = wcslen(hw->word) + 1;
                        line_num++;
                        hw->line_num = line_num;
                    }
                    /* this word fits on the current line */
                    else {
                        /* add this word to the line_width */
                        line_width = line_width + wcslen(hw->word) + 1;
                        hw->line_num = line_num;
                    }
                }
                /* end of sentence, so no trailing space */
                else if(next_hw->word == NULL && next_hw->newline == 1) {
                    /* don't add this on the current line */
                    if((int)wcslen(hw->word) >= (width - line_width)) {
                        /* set line_width to the size of this word */
                        line_width = wcslen(hw->word);
                        line_num++;
                        hw->line_num = line_num;
                    }
                    /* this word fits on the current line */
                    else {
                        /* add this word to the line_width */
                        line_width = line_width + wcslen(hw->word);
                        hw->line_num = line_num;
                    }
                }
                else {
                    vrmr_fatal("undefined state");
                }
            }
            /* end of text, doc has no trailing new-line */
            else {
                /* don't add this on the current line */
                if((int)wcslen(hw->word) >= (width - line_width)) {
                    /* set line_width to the size of this word */
                    line_width = wcslen(hw->word);
                    line_num++;
                    hw->line_num = line_num;
                }
                /* this word fits on the current line */
                else {
                    /* add this word to the line_width */
                    line_width = line_width + wcslen(hw->word);
                    hw->line_num = line_num;
                }
            }
        } else if(hw->word == NULL && hw->newline) {
            hw->line_num = line_num;
            line_num++;
            line_width = 0;
        } else {
            vrmr_fatal("undefined state");
        }
    }
}
#endif /* USE_WIDEC */

static void
do_print(const int debuglvl, WINDOW *printwin, struct vrmr_list *list,
        size_t start_print, size_t end_print)
{
    helpword    *hw = NULL,
                *next_hw = NULL;
    struct vrmr_list_node *d_node = NULL,
                *next_d_node = NULL;

    /* print the text */
    for(d_node = list->top; d_node; d_node = d_node->next)
    {
        vrmr_fatal_if_null(d_node->data);
        hw = d_node->data;

        if(hw->line_num >= start_print && hw->line_num <= end_print) {
            if(hw->word != NULL) {
                if((next_d_node = d_node->next)) {
                    vrmr_fatal_if_null(next_d_node->data);
                    next_hw = next_d_node->data;

                    /* end of sentence */
                    if(next_hw->word == NULL && next_hw->newline == 1)
                    {
                        wprintw(printwin, "%s", hw->word);
                    }
                    /* middle in sentence */
                    else if(next_hw->line_num > hw->line_num)
                    {
                        wprintw(printwin, "%s\n", hw->word);
                    }
                    /* next one is only a dot */
                    else if(next_hw->word != NULL &&
                        strcmp(next_hw->word, ".") == 0)
                    {
                        wprintw(printwin, "%s", hw->word);
                    }
                    else
                    {
                        wprintw(printwin, "%s ", hw->word);
                    }
                }
                /* end of text */
                else
                {
                    wprintw(printwin, "%s", hw->word);
                }
            }
            /* new line */
            else
            {
                wprintw(printwin, "\n");
            }
        }
    }
}

#ifdef USE_WIDEC
static void
do_wide_print(const int debuglvl, WINDOW *printwin, struct vrmr_list *list,
        int start_print, int end_print)
{
    whelpword   *hw = NULL,
                *next_hw = NULL;
    struct vrmr_list_node *d_node = NULL,
                *next_d_node = NULL;

    /* print the text */
    for(d_node = list->top; d_node; d_node = d_node->next)
    {
        vrmr_fatal_if_null(d_node->data);
        hw = d_node->data;

        if((int)hw->line_num >= start_print && (int)hw->line_num <= end_print) {
            if(hw->word != NULL) {
                if((next_d_node = d_node->next)) {
                    vrmr_fatal_if_null(next_d_node->data);
                    next_hw = next_d_node->data;

                    /* end of sentence */
                    if(next_hw->word == NULL && next_hw->newline == 1)
                    {
                        wprintw(printwin, "%ls", hw->word);
                    }
                    /* middle in sentence */
                    else if(next_hw->line_num > hw->line_num)
                    {
                        wprintw(printwin, "%ls\n", hw->word);
                    }
                    /* next one is only a dot */
                    else if(next_hw->word != NULL &&
                        wcscmp(next_hw->word, L".") == 0)
                    {
                        wprintw(printwin, "%ls", hw->word);
                    }
                    else
                    {
                        wprintw(printwin, "%ls ", hw->word);
                    }
                }
                /* end of text */
                else
                {
                    wprintw(printwin, "%ls", hw->word);
                }
            }
            /* new line */
            else
            {
                wprintw(printwin, "\n");
            }
        }
    }
}
#endif /* USE_WIDEC */

static void
print_list(const int debuglvl, struct vrmr_list *list, char *title, int height,
        int width, int starty, int startx, char utf8)
{
    WINDOW      *boxwin = NULL,
                *printwin = NULL;
    PANEL       *panel[2];
    int         ch;

    helpword    *hw = NULL;
    size_t      start_print = 1,
                end_print = 1;
    char        done = 0;
    int         i = 0;
    size_t      size = 0;

    end_print = (size_t)height - 2;

    boxwin = create_newwin(height, width, starty, startx, title, vccnf.color_win);
    vrmr_fatal_if_null(boxwin);
    panel[0] = new_panel(boxwin);
    vrmr_fatal_if_null(panel[0]);
    printwin = newwin(height-2, width-4, starty+1, startx+2);
    vrmr_fatal_if_null(printwin);
    (void)wbkgd(printwin, vccnf.color_win);
    panel[1] = new_panel(printwin);
    vrmr_fatal_if_null(panel[1]);
    keypad(printwin, TRUE);

    size = StrLen(gettext("Press <F10> to close this window."));
    mvwprintw(boxwin, height-1, (int)(width-size)/2, " %s ", gettext("Press <F10> to close this window."));
    wrefresh(boxwin);

    while(!done)
    {
        werase(printwin);

        if (list->len == 0) {
            wprintw(printwin, gettext("The requested helptext was not found.\n"));
        }

#ifdef USE_WIDEC
        if(utf8 == UTF8_TRUE)
            do_wide_print(debuglvl, printwin, list, start_print,
                    end_print);
        else
#endif /* USE_WIDEC */
            do_print(debuglvl, printwin, list, start_print,
                    end_print);
        update_panels();
        doupdate();

        /* get user input */
        ch = wgetch(printwin);
        switch(ch)
        {
            case KEY_DOWN:

                if(list->len > 0) {
                    hw = list->bot->data;
                    vrmr_fatal_if_null(hw);

                    if(end_print < hw->line_num) {
                        start_print++;
                        end_print++;
                    }
                }
                break;

            case KEY_UP:

                if(list->len > 0) {
                    if(start_print > 1) {
                        start_print--;
                        end_print--;
                    }
                }
                break;

            case KEY_PPAGE:

                if(list->len > 0) {
                    i = (height - 2)/3;

                    while((start_print - i) < 1)
                        i--;

                    start_print = start_print - i;
                    end_print = end_print - i;
                }
                break;

            case KEY_NPAGE:

                if(list->len > 0) {
                    i = (height - 2)/3;

                    hw = list->bot->data;
                    vrmr_fatal_if_null(hw);

                    while((end_print + i) > hw->line_num)
                        i--;

                    start_print = start_print + i;
                    end_print = end_print + i;
                }

                break;

            default:

                done = 1;
                break;
        }
    }

    del_panel(panel[0]);
    del_panel(panel[1]);
    destroy_win(printwin);
    destroy_win(boxwin);

    update_panels();
    doupdate();
    return;
}


void
print_help(const int debuglvl, char *part)
{
    struct vrmr_list  HelpList;
    int     max_height = 0,
            max_width = 0,
            height = 0,
            width = 0,
            startx = 0,
            starty = 0;
#ifdef USE_WIDEC
    wchar_t wpart[32] = L"";
#endif /* USE_WIDEC */

    /* get screensize */
    getmaxyx(stdscr, max_height, max_width);
    width  = 72;
    height = max_height - 6;
    startx = max_width - width - 5;
    starty = 3;

#ifdef USE_WIDEC
    if(utf8_mode == FALSE)
    {
#endif /* USE_WIDEC */
        /* read the helpfile */
        if(read_helpfile(debuglvl, &HelpList, part) < 0)
            return;
        set_lines(debuglvl, &HelpList, (size_t)(width - 4));
        print_list(debuglvl, &HelpList, gettext("Help"), height, width, starty, startx, UTF8_FALSE);
        vrmr_list_cleanup(debuglvl, &HelpList);
#ifdef USE_WIDEC
    }
    else
    {
        /* convert the part name to a wchar_t string */
        mbstowcs(wpart, part, wsizeof(wpart));
        if(debuglvl >= LOW)
            vrmr_debug(__FUNC__, "part: %s, wpart %ls, %u",
                        part, wpart, wsizeof(wpart));

        /* read the helpfile */
        if(read_wide_helpfile(debuglvl, &HelpList, wpart) < 0)
            return;
        set_wide_lines(debuglvl, &HelpList, width - 4);
        print_list(debuglvl, &HelpList, gettext("Help"), height, width, starty, startx, UTF8_TRUE);
        vrmr_list_cleanup(debuglvl, &HelpList);
    }
#endif /* USE_WIDEC */
}

void
print_status(const int debuglvl)
{
    int     max_height = 0,
            max_width = 0,
            height = 0,
            width = 0,
            startx = 0,
            starty = 0;

    /* get screensize */
    getmaxyx(stdscr, max_height, max_width);
    width  = 72;
    height = max_height - 6;
    startx = (max_width - width) / 2;
    starty = 3;

    /* should not happen */
    if(VuurmuurStatus.StatusList.len == 0) {
        (void)read_helpline(debuglvl, &VuurmuurStatus.StatusList, gettext("No problems were detected in the current setup.\n"));
    }

    set_lines(debuglvl, &VuurmuurStatus.StatusList, (size_t)(width - 4));
    /* print the status list */
    print_list(debuglvl, &VuurmuurStatus.StatusList, gettext("Status"), height, width, starty, startx, UTF8_FALSE);
}

void
setup_statuslist(const int debuglvl)
{
    /* initialize */
    memset(&VuurmuurStatus, 0, sizeof(VuurmuurStatus));
    memset(&VuurmuurStatus.StatusList, 0, sizeof(VuurmuurStatus.StatusList));

    VuurmuurStatus.vuurmuur = 1;
    VuurmuurStatus.vuurmuur_log = 1;

    VuurmuurStatus.zones = 1;
    VuurmuurStatus.interfaces = 1;
    VuurmuurStatus.services = 1;
    VuurmuurStatus.rules = 1;

    VuurmuurStatus.shm = 1;
    VuurmuurStatus.backend = 1;
    VuurmuurStatus.config = 1;
    VuurmuurStatus.settings = 1;
    VuurmuurStatus.system = 1;

    /* setup the status list */
    vrmr_list_setup(debuglvl, &VuurmuurStatus.StatusList, free_helpword);
}

void
print_about(const int debuglvl)
{
    int     max_height = 0,
            max_width = 0,
            height = 0,
            width = 0,
            startx = 0,
            starty = 0;
    struct vrmr_list  about_list;

    /* top menu */
    char    *key_choices[] =    { "F10" };
    int     key_choices_n = 1;
    char    *cmd_choices[] =    { gettext("back") };
    int     cmd_choices_n = 1;
    char    about_version_string[sizeof(version_string)];

    /* create the about version string */
    snprintf(about_version_string, sizeof(about_version_string), "Version: %s\n", version_string);

    /* get screensize */
    getmaxyx(stdscr, max_height, max_width);

    width  = 72;
    height = max_height - 8;
    startx = (max_width - width) / 2;
    starty = 4;

    vrmr_list_setup(debuglvl, &about_list, free_helpword);

    char copyright[64];
    snprintf(copyright, sizeof(copyright), "%s.\n", VUURMUUR_COPYRIGHT);
    (void)read_helpline(debuglvl, &about_list, "Vuurmuur_conf\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "=============\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, about_version_string);
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, copyright);
    (void)read_helpline(debuglvl, &about_list, "This program is distributed under the terms of the GPL2+.\n");
    (void)read_helpline(debuglvl, &about_list, "\n");

    (void)read_helpline(debuglvl, &about_list, "Support\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "=======\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "Website: http://www.vuurmuur.org/\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "Mailinglist: http://sourceforge.net/mail/?group_id=114382\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "Forum: http://sourceforge.net/forum/?group_id=114382\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "IRC: irc://irc.freenode.net/vuurmuur\n");
    (void)read_helpline(debuglvl, &about_list, "\n");

    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "Thanks to\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "=========\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "Philippe Baumgart (documentation).\n");
    (void)read_helpline(debuglvl, &about_list, "Michiel Bodewes (website development).\n");
    (void)read_helpline(debuglvl, &about_list, "Nicolas Dejardin <zephura(at)free(dot)fr> (French translation).\n");
    (void)read_helpline(debuglvl, &about_list, "Adi Kriegisch (coding, documentation, Debian packages).\n");
    (void)read_helpline(debuglvl, &about_list, "Sebastian Marten (documentation).\n");
    (void)read_helpline(debuglvl, &about_list, "Holger Ohmacht (German translation).\n");
    (void)read_helpline(debuglvl, &about_list, "Hugo Ribeiro (Brazilian Portuguese translation).\n");
    (void)read_helpline(debuglvl, &about_list, "Aleksandr Shubnik <alshu(at)tut(dot)by> (rpm development, Russian translation).\n");
    (void)read_helpline(debuglvl, &about_list, "Per Olav Siggerud (Norwegian translation).\n");
    (void)read_helpline(debuglvl, &about_list, "Alexandre Simon (coding).\n");
    (void)read_helpline(debuglvl, &about_list, "Stefan Ubbink (Gentoo ebuilds, coding).\n");
    (void)read_helpline(debuglvl, &about_list, "Rob de Wit (wiki hosting).\n");
    (void)read_helpline(debuglvl, &about_list, "\n");
    (void)read_helpline(debuglvl, &about_list, "See: http://www.vuurmuur.org/trac/wiki/Credits for the latest information.\n");
    (void)read_helpline(debuglvl, &about_list, "\n");

    set_lines(debuglvl, &about_list, (size_t)(width - 4));

    draw_top_menu(debuglvl, top_win, gettext("About"), key_choices_n, key_choices, cmd_choices_n, cmd_choices);
    /* print the status list */
    print_list(debuglvl, &about_list, gettext("About"), height, width, starty, startx, UTF8_FALSE);

    vrmr_list_cleanup(debuglvl, &about_list);
}
