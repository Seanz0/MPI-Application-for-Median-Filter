
/** \mainpage Prac3 Main Page
 *
 * \section intro_sec Introduction
 *
 * The purpose of Prac3 is to learn some basics of MPI coding.
 *
 * Look under the Files tab above to see documentation for particular files
 * in this project that have Doxygen comments.
 */



//---------- STUDENT NUMBERS --------------------------------------------------
//
// MHTSEA001-SEAN MUHITA
//
//-----------------------------------------------------------------------------

/* Note that Doxygen comments are used in this file. */
/** \file Prac3
 *  Prac3 - MPI Main Module
 *  The purpose of this prac is to get a basic introduction to using
 *  the MPI libraries for prallel or cluster-based programming.
 */

// Includes needed for the program
#include "Prac3.h"
using namespace std;

/** This is the master node function, describing the operations
    that the master will be doing */
void Master () {
 //! <h3>Local vars</h3>
 // The above outputs a heading to doxygen function entry
 int  j;             //! j: Loop counter
 char buff[BUFSIZE]; //! buff: Buffer for transferring message data
 MPI_Status stat;    //! stat: Status of the MPI application

 // Start of "Hello World" example..............................................
// printf("0: We have %d processors\n", numprocs);
// for(j = 1; j < numprocs; j++) {
//  sprintf(buff, "Hello %d! ", j);
//  MPI_Send(buff, BUFSIZE, MPI_CHAR, j, TAG, MPI_COMM_WORLD);
// }
// for(j = 1; j < numprocs; j++) {
//  // This is blocking: normally one would use MPI_Iprobe, with MPI_ANY_SOURCE,
//  // to check for messages, and only when there is a message, receive it
//  // with MPI_Recv.  This would let the master receive messages from any
//  // slave, instead of a specific one only.
//  MPI_Recv(buff, BUFSIZE, MPI_CHAR, j, TAG, MPI_COMM_WORLD, &stat);
//  printf("0: %s\n", buff);
// }
 // End of "Hello World" example................................................

 // Read the input image
 if(!Input.Read("Data/greatwall.jpg")){
  printf("Cannot read image\n");
  return;
 }
int height = Input.Height;
int wid_comp = Input.Width*Input.Components;
// Decompose image and send it to slave processors
int section = height/(numprocs-1);
int curr_section =0; //current height of the image being split
char times[BUFSIZE]; //process times from slave processors
 for(j = 1; j < numprocs; j++)
 {
     tic();
    int buffer = section*wid_comp;
    int cond;

        cond = curr_section + section;

    char buf[buffer];

    //Image data values to send to slaves
    sprintf(buff, "%d %d",section,wid_comp);
     MPI_Send(buff,BUFSIZE,MPI_CHAR,j,TAG,MPI_COMM_WORLD);
    
    //copy image
    for(int y = 0; curr_section < cond; y++, curr_section++)
     {
        for(int x = 0; x < wid_comp; x++)
        {
            buf[wid_comp*y+x] = Input.Rows[curr_section][x];
        }

    }
    //Send image data to slaves
    MPI_Send(buf,buffer,MPI_CHAR,j,TAG,MPI_COMM_WORLD);
    printf("Time taken to send data to slave %d: %lg ms \n", j,toc()*1000);
 }

 // Allocated RAM for the output image
 if(!Output.Allocate(Input.Width, Input.Height, Input.Components)) return;



curr_section = 0;
 for(j = 1; j < numprocs; j++)
 {
    int buffer = section*wid_comp;
    int cond;

         cond = curr_section + section;
     
     //Receive image data   
    char buf_rec[buffer];
    MPI_Recv(buf_rec,buffer,MPI_CHAR,j,TAG,MPI_COMM_WORLD,&stat);
    //MPI_Recv(times,BUFSIZE,MPI_CHAR,j,TAG,MPI_COMM_WORLD,&stat);
    

        //Append image data from slaves into a 2d image
       for(int y = 0; curr_section < cond; y++, curr_section++)
         {
            for(int x = 0; x < Input.Width*Input.Components; x++)
            {
                Output.Rows[curr_section][x] = buf_rec[wid_comp*y+x];
            }

        }
}
 // Write the output image
 if(!Output.Write("Data/Output.jpg")){
  printf("Cannot write image\n");
  return;
 }
 //! <h3>Output</h3> The file Output.jpg will be created on success to save
 //! the processed output.
}
//------------------------------------------------------------------------------

/** This is the Slave function, the workers of this MPI application. */
void Slave(int ID){
    char idstr[32];
    char buff [BUFSIZE];
    int height; //height of image
    int wid_comp; // Width and components of image
    int comp;
    int buffer;
    char window[49]; //window size for filtering
    int window_size = 7;
    char time[BUFSIZE]; //time taken for slave to process data
 MPI_Status stat;

    ////receive from rank 0 (master):
    MPI_Recv(buff, BUFSIZE, MPI_CHAR, 0, TAG, MPI_COMM_WORLD, &stat);
    sscanf(buff,"%d %d",&height, &wid_comp);

    buffer = wid_comp*height; // Size of incoming array
    
    char buf2[buffer]; //array to hold image data
     MPI_Recv(buf2, buffer, MPI_CHAR, 0, TAG, MPI_COMM_WORLD, &stat);

    tic();
     //window boundaries
     int y_bound = window_size/2;
     int x_bound = (window_size/2)*3;

       for(int y = y_bound; y < height-y_bound; y++)
     {
        for(int x = x_bound; x < wid_comp-x_bound; x++)
        {
            int index=0; //Used to fill the window
            for(int edge_y= -1*window_size/2; edge_y<=window_size/2; edge_y++)
            {
               for(int edge_x= (-1* window_size/2)*3; edge_x<=(window_size/2)*3; edge_x+=3, index++) //x component traversed by steps of 3 to cater for the rgb components
               {
                   window[index] = buf2[(y+edge_y)*wid_comp + (x+edge_x)];
               }
            }
        
        
        int length = sizeof(window)/sizeof(window[0]);

        //Sort the pixels in the window
        for(int t = 0; t < length; t++)
        {
            for(int q = 0; q < length-1; q++)
            {
                if(window[q] > window[q+1])
                {
                    unsigned char temp = window[q+1];
                    window[q+1] = window[q];
                    window[q] = temp;
                }
            }
        }
        
        buf2[y*wid_comp+x] = window[length/2]; //Place median of window into the image data
        }

   
    }


    MPI_Send(buf2, buffer,MPI_CHAR,0,TAG,MPI_COMM_WORLD);
    
    sprintf(time,"%lg",toc()*1000);

    MPI_Send(time, BUFSIZE, MPI_LONG_DOUBLE_INT,0,TAG,MPI_COMM_WORLD);
 

}
//------------------------------------------------------------------------------

/** This is the entry point to the program. */
int main(int argc, char** argv){
 int myid;

 // MPI programs start with MPI_Init
 MPI_Init(&argc, &argv);

 // find out how big the world is
 MPI_Comm_size(MPI_COMM_WORLD, &numprocs);

 // and this processes' rank is
 MPI_Comm_rank(MPI_COMM_WORLD, &myid);

 // At this point, all programs are running equivalently, the rank
 // distinguishes the roles of the programs, with
 // rank 0 often used as the "master".
 if(myid == 0) Master();
 else          Slave (myid);

 // MPI programs end with MPI_Finalize
 MPI_Finalize();
 return 0;
}
//------------------------------------------------------------------------------
