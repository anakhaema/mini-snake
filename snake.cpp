// Snake for MBED Application Board C12832
// Revision 237

#include <cstdlib>
#include <ctime>
#include <cmath>
#include <climits>
#include <cstdio>
#include "mbed.h"
#include "C12832_lcd.h"
#include "MMA7660.h"
#define scr_x 128
#define scr_y 32
#define THICK 2
#define F_WIDTH 2

LocalFileSystem local("local");
C12832_LCD  lcd;
MMA7660     axis(p28, p27);
BusIn       joy(p15, p12, p13, p16);
AnalogIn    pmtr_1(p19);
AnalogIn    pmtr_2(p20);
BusOut      L(LED1, LED2, LED3, LED4);
BusOut      lgt(LED3, LED4);

FILE        *file_p;
int         h_score;
float       score = 0;
int         t_rand, food_x, food_y, init_x, init_y;
int         snake_map[scr_x / THICK][scr_y / THICK] = {0};

void enumerate_table(void) {
    file_p = fopen("/local/scores.txt", "r");
    if (file_p != NULL) {
        fscanf(file_p, "%d", &h_score);
        fclose(file_p);
        remove("/local/scores.txt");
    }
}

int is_between(int number, int lower, int higher, int accuracy) {
    if (accuracy) {
        if (number >= lower && number <= higher)        {return 1;}
        else                                            {return 0;}
    } else {
        if (number > lower && number < higher)          {return 1;}
        else                                            {return 0;}
    }
}

int collision(int conv_head_0, int conv_head_1, int direction) {
    int i, j;
    int test_food_0, test_food_1;
    test_food_0 = is_between(conv_head_0, food_x, food_x + F_WIDTH, 1) || is_between(conv_head_0 + THICK, food_x, food_x + F_WIDTH, 1);
    test_food_1 = is_between(conv_head_1, food_y - F_WIDTH, food_y, 1) || is_between(conv_head_1 - THICK, food_y - F_WIDTH, food_y, 1);
    int test_wall;
    test_wall = (conv_head_1 == THICK && !direction) || (conv_head_1 == scr_y - THICK && direction == 2);
    test_wall = test_wall || (!conv_head_0 && direction == 3) || (conv_head_0 == scr_x - THICK && direction == 1);
    int test_snake, dup_1, dup_2;
    dup_1 = -1;
    dup_2 = 0;
    for (i=0; i<scr_x/2; i++) { for (j=0; j<scr_y/2; j++) {
        if (snake_map[i][j] == 3)   {dup_1++;}
        if (snake_map[i][j] > 3)    {dup_2++;}
    }}
    test_snake = (dup_1 || dup_2) ? 2 : 0;

/// Return multiple codes simultaneously
    if      (test_food_0 && test_food_1)    {return 1;} // Food
    else if (test_snake)                    {return 2;} // Snake
    else if (test_wall)                     {return 3;} // Wall
    else                                    {return 0;}
}

// Force hang
void eternal_sleep() {
    bedtime:
    L = (pmtr_1.read() / 0.125 <= 1.125) ? 15 : (pmtr_1.read() / 0.125 <= 2.25) ? 7 : (pmtr_1.read() / 0.125 <= 3.4) ? 3 : (pmtr_1.read() / 0.125 <= 4) ? 1 : 0;
//  wait(1);
    goto bedtime;
}

void print_message(int message_code, int death) {
    if (message_code == 0) {            // Game start
        lcd.locate(21, 10);
        lcd.printf("SNAKE");
        wait(0.5);
        lcd.cls();
    } else if   (message_code == 1) {   // Game end
        // Check highscores
        if (score > h_score) {h_score = score;}
        file_p = fopen("/local/scores.txt", "w");
        fprintf(file_p, "%d", h_score);
        fclose(file_p);

        lcd.cls();
        lcd.locate(7, 0);
        lcd.printf("YOU DIED         HS: %d\n  CONGRATULATIONS\n          SCORE: %.0f", h_score, score);
        eternal_sleep();
    }
}

void snake_shift(int direction, int old_direction) {
    // Declare variables
    int head[2], tail[2], i, j;

    // Determine limits of object
    for (i=0; i<scr_x/THICK; i++) {for (j=0; j<scr_y/THICK; j++) {
        if (snake_map[i][j] == 1) {
            head[0] = i;
            head[1] = j;
        } else if (snake_map[i][j] == 3) {
            tail[0] = i;
            tail[1] = j;
        }
    }}

    if (collision(2 * head[0], 2 * head[1], direction) == 3) {      // Wall collision
        print_message(1, 1);
        if      (!direction)        {snake_map[head[0]][(scr_y / THICK) - 1] = 1;}
        else if (direction == 1)    {snake_map[0][head[1]] = 1;}
        else if (direction == 2)    {snake_map[head[0]][0] = 1;}
        else if (direction == 3)    {snake_map[(scr_x / THICK) - 1][head[1]] = 1;}
        else {}
    } else if (collision(2 * head[0], 2 * head[1], direction) == 2) { // Snake collision
        print_message(1, 2);
    } else {
        if      (!direction)        {snake_map[head[0]][head[1] - 1] += 1;}     // Up
        else if (direction == 1)    {snake_map[head[0] + 1][head[1]] += 1;}     // Right
        else if (direction == 2)    {snake_map[head[0]][head[1] + 1] += 1;}     // Down
        else if (direction == 3)    {snake_map[head[0] - 1][head[1]] += 1;}     // Left
        else {}
    }

    if (collision(2 * head[0], 2 * head[1], direction) == 1) {      // Food collision
        int i, j;
        score++;

        // Clear old food
        lcd.fillrect(food_x, food_y, food_x + F_WIDTH, food_y - F_WIDTH, 0x00);

        // Randomise new values
        food_regen:
        food_x = rand() % (scr_x - 8) + 4;
        food_y = rand() % (scr_y - 4) + 2;

        // Run collision checks
        for (i=0; i<scr_x/THICK; i++) {for (j=0; j<scr_y/THICK; j++) { if (snake_map[i][j]) {
            if (abs(food_x / 2 - i) <= 2 || abs(food_y / 2 - j) <= 2) {goto food_regen;}
        }}}
    } else {
        // Clear old tail
        lcd.fillrect(2 * tail[0], 2 * tail[1], 2 * tail[0] + THICK + 1, 2 * tail[1] - THICK, 0x00);

        // Allocate new tail
        if      (snake_map[tail[0]][tail[1] + 1] == 2)  {snake_map[tail[0]][tail[1] + 1] = 3;}
        else if (snake_map[tail[0]][tail[1] - 1] == 2)  {snake_map[tail[0]][tail[1] - 1] = 3;}
        else if (snake_map[tail[0] + 1][tail[1]] == 2)  {snake_map[tail[0] + 1][tail[1]] = 3;}
        else if (snake_map[tail[0] - 1][tail[1]] == 2)  {snake_map[tail[0] - 1][tail[1]] = 3;}
        else                                            {snake_map[tail[0]][tail[1] - 1] = 3;}  // Should never be used, last resort

        // Update records
        snake_map[tail[0]][tail[1]] = 0;
    }
    // Finish updating records
    snake_map[head[0]][head[1]] = 2;
}

// Read data from joystick
int read_data() {
    if      (joy == 0x1)    {return 0;} // up
    else if (joy == 0x8)    {return 1;} // right
    else if (joy == 0x2)    {return 2;} // down
    else if (joy == 0x4)    {return 3;} // left
/*---------------------------------------*/
    else if (pmtr_2.read() > 0.25) {
        if (abs(axis.x() * 10) > abs(axis.y() * 10)) {
            if      (axis.x()*10 < 0.01)    {return 1;}
            else if (axis.x()*10 > 0.01)    {return 3;}
        } else {
            if      (axis.y()*10 > 0.01)    {return 2;}
            else if (axis.y()*10 < 0.01)    {return 0;}
            else                            {return -1;}
        }
    }
    return -1;
}

int main(void) {
    // Initialize
    wait(0.5);
    enumerate_table();
    srand(time(NULL));
    axis.setSampleRate(120);
    print_message(0, 0);

    // Error handling
    if (THICK % 2) {
        lcd.locate(0,10);
        lcd.printf("ERROR: Improper width.");
        eternal_sleep();
    }

    // Declare variables
    int i, j, old_direction, direction;
    double time_delay;
    i = init_x = init_y = old_direction = direction = L = 0;
    old_direction = 0;
    t_rand = rand() % (scr_x / THICK);
    init_x = 90;
    init_y = 20;
    food_x = food_y = 10;

    // Set snake starting position
    lcd.fillrect(init_x, init_y, init_x + THICK, init_y - (THICK * 3), 0xff);
    for (i=0; i<3; i++) {snake_map[init_x / 2][init_y / 2 - i] = (i == 2) ? 1 : (!i) ? 3 : 2;}

    // Set initial food position
    lcd.rect(food_x, food_y, food_x + F_WIDTH, food_y - F_WIDTH, 0xff);

    // Start main program loop
    i = 0;
    while (init_x < (scr_x - THICK) && init_y < (scr_y - THICK)) {
        t_rand = rand() % 10;
        time_delay = 0.03 + pmtr_1.read() / 2;

        // Show framecounter
//      lcd.locate(scr_x - 8 - 5 * floor(log10((double)i)), 0);
//      lcd.printf("%d", i++);

        // Read data from joystick
        direction = (read_data() == -1) ? old_direction : read_data();
        direction = (abs(direction - old_direction) % 2 == 0) ? old_direction : direction;

        // Move snake
        snake_shift(direction, old_direction);
        old_direction = direction;

        // Redraw objects
        lcd.cls();
        // Food
        lcd.rect(food_x, food_y, food_x + F_WIDTH, food_y - F_WIDTH, 0xff);
        // Snake
        for (i=0; i<scr_x/THICK; i++) {for (j=0; j<scr_y/THICK; j++) {
            if (snake_map[i][j] != 0) {lcd.fillrect(2 * i, 2 * j, 2 * i + THICK, 2 * j - THICK, 0xff);}
        }}

        // Decide difficulty
        L = (pmtr_1.read() / 0.125 <= 1.125) ? 15 : (pmtr_1.read() / 0.125 <= 2.25) ? 7 : (pmtr_1.read() / 0.125 <= 3.4) ? 3 : (pmtr_1.read() / 0.125 <= 4) ? 1 : 0;
        score += time_delay / 3;
        wait(time_delay);   /// Modify wait timer to recognise joystick samples (use threads?)
    }
    return 0;
}
