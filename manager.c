#include <stdio.h>
#include <stdlib.h>

#define NUM_ENTRIES 5
#define NUM_EXITS 2
#define NUM_LEVELS 3
#define CARS_PER_LEVEL 20


//Function declarations
void GenerateGUI(); char *GateStatus(char code);
void OpenGate(); void CloseGate(); 
void GenerateBill();
void ScreenController();

int main(int argc, char **argv){

    GenerateGUI();

    return 0;
}

void GenerateGUI()
{
    printf("\033[2J"); // Clear screen

    printf(
    "╔═══════════════════════════════════════════════╗\n"
    "║                CARPARK SIMULATOR              ║    by DAVID AND DANIEL \n"
    "╚═══════════════════════════════════════════════╝\n"
    );
    
    printf("\n\e[1mTotal Revenue:\e[m $0.00\n\n");


    printf("\n\e[1mLevel\tCapacity\tLPR\t\tTemp (C)\e[m\n");
    printf("══════════════════════════════════════════════════════════════════\n\n");
    for (int i = 1; i <= NUM_LEVELS; i++)
    {
        printf("%d\t", i);
        printf("%d/%d\t\t", i, CARS_PER_LEVEL);
        printf("%s\t\t", "123ABC");
        printf("%d\t\t", 27);
        printf("\n");
    }

    printf("\n");

    printf("\n\e[1mEntry\tGate\t\tLPR\t\tDisplay\e[m\n");
    printf("══════════════════════════════════════════════════════════════════\n\n");
    for (int i = 1; i <= NUM_ENTRIES; i++)
    {
        printf("%d\t", i);
        printf("%s\t\t", GateStatus('C'));
        printf("%s\t\t", "123ABC");
        printf("%c\t", '3');
        printf("\n");
    }

    printf("\n");
    
    printf("\n\e[1mExit\tGate\t\tLPR\e[m\n");
    printf("══════════════════════════════════════════════════════════════════\n\n");
    for (int i = 1; i <= NUM_ENTRIES; i++)
    {
        printf("%d\t", i);
        printf("%s\t", GateStatus('L'));
        printf("%s\t", "123ABC");
        printf("\n");
    }

    printf("\n");

}

char *GateStatus(char code)
{
    switch((int)code)
    {
        case 67:
            return "Closed";
            break;
        case 79:
            return "Open";
            break;
        case 76:
            return "Lowering";
            break;
        case 82:
            return "Raising";
            break;
        default:
            return NULL;
            break;
    }

}