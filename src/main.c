// Sample code for ECE 198

// Written by Bernie Roehl, August 2021

// This file contains code for a number of different examples.
// Each one is surrounded by an #ifdef ... #endif block inside of main().

// To run a particular example, you should remove the comment (//) in
// front of exactly ONE of the following lines:

//#define TESTING

#include <stdbool.h> // booleans, i.e. true and false
#include <stdio.h>   // sprintf() function
#include <stdlib.h>  // srand() and random() functions
#include <string.h>  // string library
#include "ece198.h"         
#include "LiquidCrystal.h"  // lcd library 

// Function Declerations
int main(void);
int translate_to_letter(char *translated_string, char *letter_in_morse, int translated_length, int translated_max);
void next_word(char *translated_string, char *letter_in_morse, int translated_length, int translated_max);
void empty_letters(char *letter_in_morse, int letter_max);
uint32_t get_morse_letter_delta(char *translated_string, char *letter_in_morse, int letter_length);
uint32_t get_space_delta(char *translated_string, char *letter_in_morse);
void end_screen (char *translated_string);
void check_if_lost ( char *translated_string, int translated_length, char **goal_message, int *goal_length, int question_num );
bool check_if_won (char *translated_string, int translated_length, char ** goal_message, int currentQusestionNumber );



// Classes declerations
typedef struct
{
    char *morse;
    char letter;
} letter;

//Constant and Global Variables
int const STANDARD_LENGTH = 200; //1 morse code bit length in ms

/* 
All timing and character information comes from 
https://en.wikipedia.org/wiki/Morse_code
*/

int main(void)
{
    HAL_Init(); // initialize the Hardware Abstraction Layer

    // Peripherals (including GPIOs) are disabled by default to save power, so we
    // use the Reset and Clock Control registers to enable the GPIO peripherals that we're using.

    __HAL_RCC_GPIOA_CLK_ENABLE(); // enable port A (for the on-board LED, for example)
    __HAL_RCC_GPIOB_CLK_ENABLE(); // enable port B (for the rotary encoder inputs, for example)
    __HAL_RCC_GPIOC_CLK_ENABLE(); // enable port C (for the on-board blue pushbutton, for example)

    // initialize the pins to be input, output, alternate function, etc...

    InitializePin(GPIOA, GPIO_PIN_5, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0); // on-board LED

    // note: the on-board pushbutton is fine with the default values (no internal pull-up resistor
    // is required, since there's one on the board)

    // set up for serial communication to the host computer
    // (anything we write to the serial port will appear in the terminal (i.e. serial monitor) in VSCode)
    
    // initialize the pins for the lcs screen
    //          gpioport        rs          rw      enable           d4         d5          d6           d7
    LiquidCrystal(GPIOB, GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_6, GPIO_PIN_10, GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_3);
    
    // set up the LCD's number of columns, rows and set cursor to (0,0) cordinate 
    begin(16,2);
    setCursor(0,0);

    // serial setup
    SerialSetup(9600);
    SerialPuts("\r \n \n");

    #ifdef TESTING
    // These are all testing lines
    char *morse = malloc(5 * sizeof(char));
    char *english = malloc(4 * sizeof(char));

    empty_letters(morse, 5);

    morse[0] = '-';
    morse[1] = '-';
    morse[2] = '-';

    translate_to_letter(english, morse, 0, 5);
    SerialPuts(morse);
    SerialPutc('\n');
    SerialPuts(english);
    SerialPutc('\n');

    empty_letters(morse, 5);

    morse[0] = '.';
    morse[1] = '.';
    morse[2] = '.';

    translate_to_letter(english, morse, 1, 5);
    print(english);
    SerialPuts(morse);
    SerialPutc('\n');
    SerialPuts(english);
    SerialPutc('\n');
    
    empty_letters(letter_in_morse, letter_max);
    letter_length = 0;
    
    #endif

    // as mentioned above, only one of the following code sections will be used
    // (depending on which of the #define statements at the top of this file has been uncommented)

    //these strings store the letters and final message
    int letter_length = 0;
    int letter_max = 5;
    char *letter_in_morse = malloc(letter_max * sizeof(char)); //4 is max morse code char value + null 
    empty_letters(letter_in_morse, letter_max);
    int translated_length = 0;
    int translated_max = 20;
    char *translated_string = malloc(translated_max * sizeof(char)); //20 is arbitrary, depends on the final message
    empty_letters(translated_string, translated_max);

    int num_Questions = 5;
    char *goal_message [5] = {"HA\0", "DOG\0", "GREEN\0", "ECE\0", "LCD\0"};
    int goal_length [5] = {2,3,5,3,3};
    bool rightAnswerEnterd = false;

    for ( int currentQusestionNumber=0; currentQusestionNumber<num_Questions; currentQusestionNumber++)
    {
        //this waits until the first press
        while (!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)) {} 
        while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13)){}

        while (!rightAnswerEnterd)
        {
            // This is the morse code translator
            uint32_t delta = get_morse_letter_delta(translated_string, letter_in_morse, letter_length);

            //this is to catch mispresses
            if (delta < STANDARD_LENGTH / 2)
            {
                continue;
            }
            
            //this is for dots -> [1/2 standard, 3*standard]
            if (delta < STANDARD_LENGTH * 3)
            {
                letter_in_morse[letter_length] = '.';    
            }
            //this is for dashes -> 3*standard +
            else
            {
                letter_in_morse[letter_length] = '-';
            }

            // increment the morse char
            if (letter_length < letter_max-1)
            {
                letter_length++;
            }

            // This is spaces
            delta = get_space_delta(translated_string, letter_in_morse);

            //this means that its the same letter
            if (delta < 3 * STANDARD_LENGTH)
            {  
                SerialPuts("\rNext Morse Confirmed \n");
                
                // if user enters more than more than 4 char --> they have to re-enter the morse letter
                if ( letter_length == 4 )
                {
                    empty_letters(letter_in_morse, letter_max);
                    letter_length = 0;
                }

                continue;
            }
            else
            {
                //this means the next letter -> [3*standard, 7*standard]
                if (delta < STANDARD_LENGTH * 7)
                {
                    int result = translate_to_letter(translated_string, letter_in_morse, translated_length, translated_max);
                    // If this function returns 1 it means that it didn't add a letter
                    // Code goes back to input
                    if (result == 1)
                    {
                        SerialPuts("\rLetter was not found \n");
                        empty_letters(letter_in_morse, letter_max);
                        letter_length = 0;
                        continue;
                    }
                    else
                    {
                        translated_length ++;
                        SerialPuts("\rNext letter confirmed \n");
                    }
                }
                //  7*standard + 
                else
                {
                    // the letter before is also a space
                    if ( translated_string[translated_length-1] == ' ' || translated_string[translated_length-1] == '\0' )
                    {
                        SerialPuts("\nExtra space\n");
                        empty_letters(letter_in_morse, letter_max);
                        letter_length = 0;
                        continue;
                    }

                    // onto the next word
                    else 
                    {
                        SerialPuts("\rSPACE  Confirmed \n");
                        next_word(translated_string, letter_in_morse, translated_length, translated_max);
                        translated_length += 2;
                    }
                }
                if (check_if_won(translated_string, translated_length-1, goal_message, currentQusestionNumber) )
                {
                    clear();
                    print("Correct !");
                    SerialPuts("   Target string matched!");
                    SerialPuts("\n\n"); 
                    rightAnswerEnterd = true;

                    if ( currentQusestionNumber == num_Questions )
                    {
                        HAL_Delay(500);

                        SerialPuts("======================================");
                        SerialPuts("\tCongrats! You Won!");
                        print("YOU WON");
                        setCursor(0, 1);
                        print("Reset to play");
                        
                        while(true){}
                    }
                }
                else
                {
                    check_if_lost( translated_string, translated_length-1, goal_message, goal_length, currentQusestionNumber );

                    SerialPuts(letter_in_morse);
                    SerialPutc('\n');

                    // reset the morse chars
                    empty_letters(letter_in_morse, letter_max);
                    letter_length = 0;
                    
                    clear();
                    print(translated_string);
                    SerialPuts("Final Print :");
                    SerialPuts(translated_string);
                    SerialPutc('\n');
                }
            }
        }
        // reset the variables for the next questions 
        rightAnswerEnterd = false;
        SerialPuts("Next question! \n\n");
        empty_letters(letter_in_morse, letter_max);
        empty_letters(translated_string, translated_max);
        translated_length = 0;
        letter_length = 0;
    }

    return 0;
}

void empty_letters(char *letter_in_morse, int letter_max)
{
    for (int i = 0; i < letter_max; i++)
    {
        letter_in_morse[i] = '\0';
    }

    return;
}

uint32_t get_morse_letter_delta(char *translated_string, char *letter_in_morse, int letter_length)
{
    //this is for dots and dashes
    uint32_t start = HAL_GetTick();
    uint32_t end;

    // turns the onboard LED on indicating button press registery
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET);
    bool oncePrinted = false;

    while (!HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13))
    {
        end = HAL_GetTick();
        if ( end - start < STANDARD_LENGTH * 3 && end - start > STANDARD_LENGTH/2)
        {
            SerialPuts("\r  Dot");

            if (!oncePrinted)
            {
                oncePrinted = true;
                clear();
                print(translated_string);
                setCursor(0, 1);
                print(letter_in_morse);
                setCursor(letter_length, 1);
                print(".");
            }
        }
        else if (end - start > STANDARD_LENGTH * 3)
        {
            SerialPuts("\r Dash");

            if (oncePrinted)
            {
                oncePrinted = false;
                clear();
                print(translated_string);
                setCursor(0, 1);
                print(letter_in_morse);
                setCursor(letter_length, 1);
                print("-");
            }
        }
    }

    
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);

    end = HAL_GetTick();
    return end - start;
}

uint32_t get_space_delta(char *translated_string, char *letter_in_morse)
{
    //this is for spaces
    uint32_t start = HAL_GetTick();
    uint32_t end;
    bool oncePrinted = false;

    SerialPutc('\n');
        
    while (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13))
    {
        end = HAL_GetTick();
        if ( end - start < 3*STANDARD_LENGTH ){
            SerialPuts("\rNext Morse ");

            if (!oncePrinted)
            {
                oncePrinted = true;
                clear();
                print(translated_string);
                setCursor(0, 1);
                print(letter_in_morse);
                setCursor(5, 1);
                print("|");
            }
        }

        else if ( end - start < 7*STANDARD_LENGTH ){
            SerialPuts("\rNext Letter ");
            
            if (oncePrinted)
            {
                oncePrinted = false;
                clear();
                print(translated_string);
                setCursor(0, 1);
                print(letter_in_morse);
                setCursor(5, 1);
                print("||");
            }
        }
    
        else{
            SerialPuts("\rSPACE        ");

            if (!oncePrinted)
            {
                oncePrinted = true;
                clear();
                print(translated_string);
                setCursor(0, 1);
                print(letter_in_morse);
                setCursor(5, 1);
                print("|||");
            }
        }
    }

    end = HAL_GetTick();
    return end - start;
}

void next_word(char *translated_string, char *letter_in_morse, int  translated_length, int translated_max)
{
    int result = translate_to_letter(translated_string, letter_in_morse, translated_length, translated_max);
    
    if (result == 1)
    {
        SerialPuts("\nLetter was not found No Space \n");
        translated_string[translated_length] = '\0';
    }
    else
    {
        translated_string[translated_length+1] = ' ';
    }
    
    return;
}

int translate_to_letter(char *translated_string, char *letter_in_morse, int translated_length, int translated_max)
{
    letter translation[26] = {
        {".-", 'A'},
        {"-...", 'B'},
        {"-.-.", 'C'},
        {"-..", 'D'},
        {".", 'E'},
        {"..-.", 'F'},
        {"--.", 'G'},
        {"....", 'H'},
        {"..", 'I'},
        {".---", 'J'},
        {"-.-", 'K'},
        {".-..", 'L'},
        {"--", 'M'},
        {"-.", 'N'},
        {"---", 'O'},
        {".--.", 'P'},
        {"--.-", 'Q'},
        {".-.", 'R'},
        {"...", 'S'},
        {"-", 'T'},
        {"..-", 'U'},
        {"...-", 'V'},
        {".--", 'W'},
        {"-..-", 'X'},
        {"-.--", 'Y'},
        {"--..", 'Z'},
    };

    for (int i = 0; i < 26; i++)
    {
        if (strcmp(letter_in_morse, translation[i].morse) == 0)
        {
            translated_string[translated_length] = translation[i].letter;
            return 0;
        }
    }
    return 1;
}

void end_screen ( char *translated_string )
{
    clear();
    print("U lost");
    setCursor(0, 1);
    print("Reset the board");
    SerialPuts("You lost");
    SerialPutc('\n');
    SerialPuts(translated_string);
    SerialPutc('\n');

    while (true){}
}

bool check_if_won (char *translated_string, int translated_length, char ** goal_message, int currentQusestionNumber ){
    bool changed = false;
    if (translated_string[translated_length] == ' ' )
    {
        changed = true;
        // because it spaces also count as length but should not be considered when at the end of string
        translated_string[translated_length] = '\0';
    }
    
    if (strcmp(translated_string, goal_message[currentQusestionNumber]) == 0)
    {
        if ( changed )
        {
            translated_string[translated_length] = ' ';
        }
        return true;
    }
    
    if ( changed )
    {
        translated_string[translated_length] = ' ';
    }
    
    return false;

}

void check_if_lost ( char *translated_string, int translated_length, char **goal_message, int *goal_length, int question_num )
{
    if (translated_string[translated_length] == ' ' )
    {
        // because it spaces also count as length but should not be considered when at the end of string
        translated_length --;
    }

    // users answer is wrong b/c they entered too many characters and the characters they entered dont match the required
    // they loose the game and must restart to retry.
    if (translated_length >= goal_length[question_num] )
    {
        end_screen(translated_string);
    }

    // if the translated index ( that has been just entered ) does not match the goal user lost
    char * val = goal_message[question_num];
    if ( translated_string[translated_length] != val[translated_length] )
    {
        end_screen(translated_string);
    }

}
// This function is called by the HAL once every millisecond
void SysTick_Handler(void)
{
    HAL_IncTick(); // tell HAL that a new tick has happened
    // we can do other things in here too if we need to, but be careful
}
