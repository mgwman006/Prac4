
/*
 * EEE3096S
 * Prac4.cpp
 * 
 * MANENO MGWAMI (MGWMAN006) AND KEVIN PIAO (PXXZHE001)
 */

#include "Prac4.h"

using namespace std;

bool playing = true; // should be set false when paused
bool stopped = false; // If set to true, program should close
unsigned char buffer[2][BUFFER_SIZE][2];
int buffer_location = 0;
bool bufferReading = 0; //using this to switch between column 0 and 1 - the first column
bool threadReady = false; //using this to finish writing the first column at the start of the song, before the column is played



long lastInterruptTime =0 ;

void play(void)
{
     long interrupt_time = millis();
     if ( interrupt_time - lastInterruptTime > 60 )
     { playing = !playing;}
     lastInterruptTime=interrupt_time;

}

void stop(void)
{
     long interrupt_time = millis();
     if ( interrupt_time - lastInterruptTime > 60 )
     {
        //cout<<"stopped"<<endl;
       stopped = true;
       exit(0);
     }
    lastInterruptTime=interrupt_time;

}
int setup_gpio(void){
    //Set up wiring Pi
     wiringPiSetup();
    //setting up the buttons
    pinMode(PLAY_BUTTON, INPUT);
    pullUpDnControl(PLAY_BUTTON, PUD_UP);
    pullUpDnControl(STOP_BUTTON, PUD_UP);
    wiringPiISR(PLAY_BUTTON,INT_EDGE_FALLING,play);
    wiringPiISR(STOP_BUTTON,INT_EDGE_FALLING,stop);

    //setting up the SPI interface
    wiringPiSPISetup(SPI_CHAN, SPI_SPEED);
    return 0;
}

/*
 * Thread that handles writing to SPI
 *
 * You must pause writing to SPI if not playing is true (the player is paused)
  * When calling the function to write to SPI, take note of the last argument.
 * You don't need to use the returned value from the wiring pi SPI function
 * You need to use the buffer_location variable to check when you need to switch buffers
 */
void *playThread(void *threadargs){
    // If the thread isn't ready, don't do anything
    while(!threadReady)
        continue;

    //You need to only be playing if the stopped flag is false
    while(!stopped){
        //Code to suspend playing if paused
        //TODO
        while(!playing)
            continue;

        //Write the buffer out to SPI
        //TODO
        wiringPiSPIDataRW (SPI_CHAN, buffer[bufferReading][buffer_location], 2) ;

        //Do some maths to check if you need to toggle buffers
        buffer_location++;
        if(buffer_location >= BUFFER_SIZE) {
            buffer_location = 0;
            bufferReading = !bufferReading; // switches column one it finishes one column
        }
    }

    pthread_exit(NULL);
}

int main(){
    // Call the setup GPIO function
        if(setup_gpio()==-1){
        return 0;
    }

    /* Initialize thread with parameters
     * Set the play thread to have a 99 priority
     * Read https://docs.oracle.com/cd/E19455-01/806-5257/attrib-16/index.html
          */

    //Write your logic here
    pthread_attr_t tattr;
    pthread_t thread_id;
    int newprio = 99;
    sched_param param;

    pthread_attr_init (&tattr); /* initialized with default attributes */
    pthread_attr_getschedparam (&tattr, &param); /* safe to get existing scheduling param */
    param.sched_priority = newprio; /* set the priority; others are unchanged */
    pthread_attr_setschedparam (&tattr, &param); /* setting the new scheduling param */
    pthread_create(&thread_id, &tattr, playThread, (void *)1); /* with new priority specified *
    /*
     * Read from the file, character by character
     * You need to perform two operations for each character read from the file
     * You will require bit shifting
     *
     * buffer[bufferWriting][counter][0] needs to be set with the control bits
     * as well as the first few bits of audio
     *
     * buffer[bufferWriting][counter][1] needs to be set with the last audio bits
     *
     * Don't forget to check if you have pause set or not when writing to the buffer
     *
     */

    // Open the file
    char ch;
    FILE *filePointer;
    printf("%s\n", FILENAME);
    filePointer = fopen(FILENAME, "r"); // read mode

    if (filePointer == NULL) {
        perror("Error while opening the file.\n");
        exit(EXIT_FAILURE);
    }

    int counter = 0;
    int bufferWriting = 0;

    // Have a loop to read from the file
    while((ch = fgetc(filePointer)) != EOF){
        while(threadReady && bufferWriting==bufferReading && counter==0){
            //waits in here after it has written to a side, and the thread is still reading from the other side
            continue;
        }

        // Bit shifting is used so that the 8 bits you read from the file can be correctly distributed
        // among the 16 bits that you have to write over SPI.
        // The way the bits are laid out, and what all the flags mean, can be found in the datasheet.

        //Set config bits for first 8 bit packet and OR with upper bits

        char value = fgetc(filePointer);
       
        char upper = ((value >> 6) & 0xff) | 0b0 << 7 | 0b0 << 6 | 0b1 << 5 | 0b1 << 4;
        char lower = value << 2 & 0b11111100;
       
        buffer[bufferWriting][counter][0] = upper; //TODO
     
        buffer[bufferWriting][counter][1] = lower; //TODO

        counter++;
        if(counter >= BUFFER_SIZE+1){
            if(!threadReady){
                threadReady = true;
            }

            counter = 0;
            bufferWriting = (bufferWriting+1)%2;
        }
    }

    // Close the file
    fclose(filePointer);
    printf("Complete reading");
        //Join and exit the playthread
    pthread_join(thread_id, NULL);
    pthread_exit(NULL);

    return 0;
}
