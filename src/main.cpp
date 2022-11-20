#include <ncurses.h>
#include <fstream>
#include <vector>

#define back_color 1
#define file_color 2
#define info_color 3
#define cmd_color 4

int line_number, column_number, line_down_limit, line_up_limit, column_left_limit, column_right_limit, max_line_number;
WINDOW *win, *win1, *win2;
std :: vector<char> line[10005];
struct Coordinate
{
    int x, y;
    Coordinate() { x = y = 0; }
    void add(int delta_x, int delta_y) { x += delta_x, y += delta_y; }
} cursor, screen_cursor, last_cursor;

namespace Screen_operations
{
    void Print_Cursor_Position() { wclear(win1), wprintw(win1, "%d, %d", cursor.x, cursor.y); }
    void Update_Cursor_Position() 
    {
        Print_Cursor_Position();
        wmove(win, cursor.x - line_up_limit, cursor.y - column_left_limit);
        wrefresh(win1), wrefresh(win);
    }
    void Print_To_Screen()
    {
        wmove(win, 0, 0), wclear(win);
        for(int i = line_up_limit; i <= line_down_limit; i++)
        {
            bool flag = 0;
            for(int j = column_left_limit; j <= std :: min(column_right_limit, (int) line[i].size() - 1); j++)
                flag = 1, wprintw(win, "%c", line[i][j]);
            if(!flag && i < line_down_limit) wmove(win, i - line_up_limit + 1, 0);
        }
        Update_Cursor_Position();
    }
    void Print_Page_Up_Down(int opt)
    {
        line_up_limit += opt, line_down_limit += opt;
        Print_To_Screen();
    }
    void Print_Page_Left_Right(int opt)
    {
        column_left_limit += opt, column_right_limit += opt;
        Print_To_Screen();
    }
    bool Check_Cursor()
    {
        bool flag = 1;
        if(cursor.x < line_up_limit) Print_Page_Up_Down(cursor.x - line_up_limit), flag = 0;
        if(cursor.x > line_down_limit) Print_Page_Up_Down(cursor.x - line_down_limit), flag = 0;
        if(cursor.y < column_left_limit) Print_Page_Left_Right(cursor.y - column_left_limit), flag = 0;
        if(cursor.y > column_right_limit) Print_Page_Left_Right(cursor.y - column_right_limit), flag = 0;
        return flag;
    }
    void Line_Add_Char(int ch)
    {
        line[cursor.x].push_back(ch);
        for(int i = line[cursor.x].size() - 1; i > cursor.y; i--) line[cursor.x][i] = line[cursor.x][i - 1];
        line[cursor.x][cursor.y] = ch;
        wmove(win, cursor.x - line_up_limit, 0);
        for(auto c : line[cursor.x]) wprintw(win, "%c", c);
    }
    void Line_Delete_Char()
    {
        for(int i = cursor.y - 1; i < line[cursor.x].size() - 1; i++) line[cursor.x][i] = line[cursor.x][i + 1];
        line[cursor.x].pop_back();
    }
    void Print_Input(int ch)
    {
        if(ch == 9)
        {
            for(int i = 0; i < 4; i++)
            {
                Line_Add_Char(' '), cursor.y++;
                if(Check_Cursor()) Update_Cursor_Position();
            }
        }
        else if(ch != 10 && ch != 13)
        {
            Line_Add_Char(ch), cursor.y++;
            if(Check_Cursor()) Update_Cursor_Position();
        }
        else
        {
            Line_Add_Char('\n'), cursor.y++;
            if(cursor.y != line[cursor.x].size())
            {
                max_line_number++;
                for(int i = max_line_number; i >= cursor.x + 2; i--) line[i] = line[i - 1];
                line[cursor.x + 1].clear();
                for(int i = cursor.y; i < line[cursor.x].size(); i++) line[cursor.x + 1].push_back(line[cursor.x][i]);
                for(int i = line[cursor.x].size() - 1; i >= cursor.y; i--) line[cursor.x].pop_back();
            }
            else if(line[cursor.x].size() > 1 && line[cursor.x][cursor.y - 1] == '\n' && line[cursor.x][cursor.y - 2] == '\n')
            {
                max_line_number++;
                for(int i = max_line_number; i >= cursor.x + 2; i--) line[i] = line[i - 1];
                line[cursor.x + 1].clear(), line[cursor.x + 1].push_back('\n');
                line[cursor.x].pop_back();
            }
            cursor.x++, cursor.y = 0;
            Print_To_Screen();
            max_line_number = std :: max(max_line_number, cursor.x);
            if(Check_Cursor()) Update_Cursor_Position();
        }
    }
    void Print_Backspace()
    {
        if(cursor.y)
        {
            Line_Delete_Char(), cursor.y--, Print_To_Screen();
            if(Check_Cursor()) Update_Cursor_Position();
        }
        else
        {
            if(cursor.x)
            {
                cursor.y = line[cursor.x - 1].size() - 1, line[cursor.x - 1].pop_back();
                for(auto c : line[cursor.x]) line[cursor.x - 1].push_back(c);
                for(int i = cursor.x; i < max_line_number; i++) line[i] = line[i + 1];
                line[max_line_number].clear(), max_line_number--;
                cursor.x--, Print_To_Screen();
                if(Check_Cursor()) Update_Cursor_Position();
            }
        }
    }
    void Print_Key_Up()
    {
        if(cursor.x)
        {
            cursor.x--;
            if(line[cursor.x].size() < cursor.y)
            {
                cursor.y = line[cursor.x].size() - 1;
                if(Check_Cursor()) Update_Cursor_Position();
                if(cursor.y < column_number) Print_Page_Left_Right(-column_left_limit);
            }
            else if(Check_Cursor()) Update_Cursor_Position();
        }
    }
    void Print_Key_Down()
    {
        if(cursor.x < max_line_number)
        {
            cursor.x++;
            if(line[cursor.x].size() < cursor.y)
            {
                cursor.y = line[cursor.x].size() - (cursor.x != max_line_number);
                if(Check_Cursor()) Update_Cursor_Position();
                if(cursor.y < column_number) Print_Page_Left_Right(-column_left_limit);
            }
            else if(Check_Cursor()) Update_Cursor_Position();
        }
    }
    void Print_Key_Left()
    {
        if(!cursor.y)
        {
            if(cursor.x) cursor.x--, cursor.y = line[cursor.x].size() - 1;
        }
        else cursor.y--;
        if(Check_Cursor()) Update_Cursor_Position();
    }
    void Print_Key_Right()
    {
        if(cursor.y == line[cursor.x].size() - (cursor.x != max_line_number))
        {
            if(cursor.x != max_line_number) cursor.y = 0, cursor.x++;
        }
        else cursor.y++;
        if(Check_Cursor()) Update_Cursor_Position();
    }
}
using namespace Screen_operations;

namespace Input_processing
{
    int Check_Input_Char(int ch)
    {
        if(ch == 27) return 0;
        if(ch == 9 || ch == 10 || ch == 13) return 1;
        if(ch >= 32 && ch <= 57) return 1;
        if(ch >= 59 && ch <= 126) return 1;
        if(ch == KEY_BACKSPACE || ch == 127 || ch == '\b') return 2;
        if(ch == KEY_UP) return 3;
        if(ch == KEY_DOWN) return 4;
        if(ch == KEY_LEFT) return 5;
        if(ch == KEY_RIGHT) return 6;
    }
}
using namespace Input_processing;

int main(int argc, char *argv[])
{ 
    // init the screen
    initscr(), raw(), noecho();

    // init color
    start_color();
    init_pair(back_color, COLOR_WHITE, COLOR_BLACK);
    init_pair(file_color, COLOR_WHITE, COLOR_BLACK);
    init_pair(info_color, COLOR_WHITE, COLOR_CYAN);
    init_pair(cmd_color, COLOR_WHITE, COLOR_RED);

    // set stdscr color
    wbkgd(stdscr, COLOR_PAIR(back_color));
    keypad(stdscr, 1), wrefresh(stdscr);

    line_number = LINES, column_number = COLS;
    if (line_number < 10) {
        fprintf(stderr, "window line size is small than 8");
        endwin();
        return 0;
    }
    line_down_limit = line_number - 3, line_up_limit = 0;
    column_left_limit = 0, column_right_limit = column_number - 1;

    win = newwin(line_number - 2, COLS, 0, 0);
    win1 = newwin(1, COLS, line_number - 2, 0);
    win2 = newwin(1, COLS, line_number - 1, 0);
    wbkgd(win, COLOR_PAIR(file_color)), keypad(win, 1);
    wbkgd(win1, COLOR_PAIR(info_color)), keypad(win1, 1);
    wbkgd(win2, COLOR_PAIR(cmd_color)), keypad(win2, 1);
    wrefresh(win), wrefresh(win1), wrefresh(win2);

    wmove(win, 0, 0), wrefresh(win);

    int ch = 0, flag;
    while (1) 
    {
        ch = getch(), flag = Check_Input_Char(ch);
        switch(flag)
        {
            case 1: Print_Input(ch); break;
            case 2: Print_Backspace(); break;
            case 3: Print_Key_Up(); break;
            case 4: Print_Key_Down(); break;
            case 5: Print_Key_Left(); break;
            case 6: Print_Key_Right(); break;
        }
        if(!flag) break;
    }

    endwin(); /* End curses mode */
    return 0;
}
