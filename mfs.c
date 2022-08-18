
#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdint.h>
#include <ctype.h>

#define MAX_NUM_ARGUMENTS 5

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

struct __attribute__((__packed__)) DirectoryEntry {
  char      DIR_Name[11];
  uint8_t   DIR_Attr;
  uint8_t   Unused1[8];
  uint16_t  DIR_FirstClusterHigh;
  uint8_t   Unused2[4];
  uint16_t  DIR_FirstClusterLow;
  uint32_t  Dir_FileSize;
};

struct DirectoryEntry dir[16];

int LBAToOffset (int32_t sector);
int compare (char *DIR_Name, char *input );


int16_t  BPB_BytsPerSec;
int8_t   BPB_SecPerClus;
int16_t  BPB_RsvdSecCnt;
int8_t   BPB_NumFATS;
int32_t  BPB_FATSz32;
int32_t  Dir;


int main()
{
  FILE *fp = NULL;
  FILE *filep = NULL;

  fp = fopen("fat32.img","r");

    fseek( fp, 11, SEEK_SET );
    fread( &BPB_BytsPerSec, 2, 1, fp );

    fseek( fp, 13, SEEK_SET );
    fread( &BPB_SecPerClus, 1, 1, fp );

    fseek( fp, 14, SEEK_SET );
    fread( &BPB_RsvdSecCnt, 2, 1, fp );

    fseek( fp, 16, SEEK_SET );
    fread( &BPB_NumFATS, 1, 1, fp );

    fseek( fp, 36, SEEK_SET );
    fread( &BPB_FATSz32, 4, 1, fp );

    fclose(fp);

    int Dir = ( BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec) +
                       (BPB_RsvdSecCnt * BPB_BytsPerSec);

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the mfs prompt
    printf ("mfs> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    if ( strcmp(cmd_str, "\n") == 0 )
    {
      continue;
    }

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;

    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;

    char *working_str  = strdup( cmd_str );

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) &&
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }


    if ( strcmp(token[0],"exit" ) == 0 || strcmp(token[0],"quit" ) == 0)
    {
      exit(0); // this will quit program
    }
    else if ( strcmp(token[0],"bpb" ) == 0 )
    {
      if ( filep == NULL )
      {
        printf("Error: File system image must be opened first.\n");
      }
      else
      {
        printf("BPB_BytsPerSec: %d\n", BPB_BytsPerSec);
        printf("BPB_BytsPerSec: %x\n\n", BPB_BytsPerSec);

        printf("BPB_SecPerClus: %d\n", BPB_SecPerClus);
        printf("BPB_SecPerClus: %x\n\n", BPB_SecPerClus);

        printf("BPB_RsvdSecCnt: %d\n", BPB_RsvdSecCnt);
        printf("BPB_RsvdSecCnt: %x\n\n", BPB_RsvdSecCnt);

        printf("BPB_NumFATS:    %d\n", BPB_NumFATS);
        printf("BPB_NumFATS:    %x\n\n", BPB_NumFATS);

        printf("BPB_FATSz32:    %d\n", BPB_FATSz32);
        printf("BPB_FATSz32:    %x\n\n", BPB_FATSz32);
      }
    }
    else if ( strcmp(token[0],"open" ) == 0 )
    {

      if ( filep != NULL )
      {
        printf("Error: File system image already open.\n");
      }
      else if ( token[1] == NULL)
      {
        printf("Error: Please use right format.\n");
      }
      else
      {

        filep = fopen(token[1],"r");

        if ( filep == NULL )
        {
          printf("Error: File system image not found.\n");
        }
        else
        {
          fseek( filep, Dir, SEEK_SET );
          fread( dir, 16, sizeof( struct DirectoryEntry), filep );
        }
      }

    }
    else if ( strcmp(token[0],"close" ) == 0 )
    {

      if ( filep == NULL )
      {
        printf("Error: File system not open.\n");
      }
      else
      {
        fclose(filep);
        filep = NULL;
      }

    }
    else if ( strcmp(token[0],"cd" ) == 0)
    {
      if ( filep == NULL )
      {
        printf("Error: File system image must be opened first.\n");
      }
      else if ( token[1] == NULL)
      {
        printf("Error: Please use right format.\n");
      }
      else
      {
        int i,flag = 0;


        fseek( filep, Dir, SEEK_SET );
        fread( dir, 16, sizeof( struct DirectoryEntry), filep );
        if ( strcmp(token[1],".." ) == 0)
        {
          for (i=0; i<16; i++)
          {
            char * input = (char*) malloc( strlen(token[1])+1 );
            memset( input, 0, strlen( token[1] ) + 1 );
            strncpy( input,  dir[i].DIR_Name , strlen( token[1] ) );

            if ( strcmp( input, token[1] ) == 0 )
            {
              if( dir[i].DIR_FirstClusterLow == 0 )
              {
                dir[i].DIR_FirstClusterLow = 2;
              }
              Dir = LBAToOffset(dir[i].DIR_FirstClusterLow);
              flag =1;
            }
            free ( input );
          }
        }
        else
        {
          for (i=0; i<16; i++)
          {
            char * input = (char*) malloc( strlen(token[1])+1 );
            memset( input, 0, strlen( token[1] ) + 1 );
            strncpy( input, token[1], strlen( token[1] ) );

            if ( compare( dir[i].DIR_Name, input ) && ( dir[i].DIR_Attr == 0x10 ) )
            {
              if( dir[i].DIR_FirstClusterLow == 0 )
              {
                dir[i].DIR_FirstClusterLow = 2;
              }
              Dir = LBAToOffset(dir[i].DIR_FirstClusterLow);
              flag =1;
            }
            free ( input );
          }

        }

        if (flag == 0 )
        {
          printf("Error: Unable to find directory %s\n", token[1]);
        }
      }
    }
    else if ( strcmp(token[0],"read" ) == 0 )
    {
      if ( filep == NULL )
      {
        printf("Error: File system image must be opened first.\n");
      }
      else if ( token[1] == NULL || token[2] == NULL || token[3] == NULL  )
      {
        printf("Error: Please use right format.\n");
      }
      else if ( filep != NULL )
      {

        unsigned char read_file[255];
        int i, j,size, flag = 0;

        fseek( filep, Dir, SEEK_SET );
        fread( dir, 16, sizeof( struct DirectoryEntry), filep );
        memset( read_file, 0, strlen( read_file ) + 1  );

        for (j = 0; j < 16; j++ )
        {
          char * input = (char*) malloc( strlen(token[1])+1 );
          memset( input, 0, strlen( token[1] ) + 1 );
          strncpy( input, token[1], strlen( token[1] ) );

          if ( compare( dir[j].DIR_Name, input ) && ( dir[j].DIR_Attr == 0x20 ) )
          {
            int D = LBAToOffset(dir[j].DIR_FirstClusterLow);
            fseek( filep, D + atoi(token[2]), SEEK_SET );

            size = atoi(token[3]) - atoi(token[2]);

            while ( size >= 512 )
            {

              memset( read_file, 0, 256 );
              fread( read_file, 255, 1, filep );

              for( i = 0 ; i <  255; i++ )
              {
                printf("%d ",read_file[i]);
              }

              size = size - 255;
            }

            fread( read_file, size, 1, filep );
            flag =1;

            for( i = 0 ; i < size; i++ )
            {
              printf("%d ",read_file[i]);
            }
            printf("\n");

          }
          free ( input );
        }

        if (flag == 0 )
        {
          printf("Error: File not found.\n");
        }
      }

    }

    else if ( strcmp(token[0],"ls" ) == 0 )
    {
      if ( filep == NULL )
      {
        printf("Error: File system image must be opened first.\n");
      }
      else
      {

        int i;

        fseek( filep, Dir, SEEK_SET );
        fread( dir, 16, sizeof( struct DirectoryEntry), filep );

        for(i=0; i < 16; i++ )
        {
          if ( (dir[i].DIR_Attr == 0x01 || dir[i].DIR_Attr == 0x10 ||
             dir[i].DIR_Attr == 0x20 ) && ( dir[i].DIR_Name[0] != 0xffffffe5 ) )
          {

            dir[i].DIR_Name[11] = '\0';
            printf("%s\n", dir[i].DIR_Name);
          }

        }
      }

    }
    else if ( strcmp(token[0],"stat" ) == 0 )
    {
      if ( filep == NULL )
      {
        printf("Error: File system image must be opened first.\n");
      }
      else if ( token[1] == NULL)
      {
         printf("Error: Please use right format.\n");
      }
      else
      {
        int i, size, flag = 0;
        fseek( filep, Dir, SEEK_SET );
        fread( dir, 16, sizeof( struct DirectoryEntry), filep );

        for (i=0; i<16; i++)
        {

          char * input = (char*) malloc( strlen(token[1])+1 );
          memset( input, 0, strlen( token[1] ) + 1 );
          strncpy( input, token[1], strlen( token[1] ) );
          if ( compare( dir[i].DIR_Name, input ) )
          {
            if (dir[i].DIR_Attr == 0x10 )// this is for directory
            {
              size = 0;
            }
            else
            {
              size = dir[i].Dir_FileSize;
            }
            printf("File Attribute      Size      Starting Cluster Number\n");
            printf("%d                  %d          %d\n",dir[i].DIR_Attr,
                    size ,dir[i].DIR_FirstClusterLow);
            flag =1;
          }
           free ( input );
        }

        if (flag == 0 ) printf("Error: File not found.\n");
      }

    }
    else if (strcmp(token[0],"get" ) == 0 )
    {
      if ( filep == NULL )
      {
        printf("Error: File system image must be opened first.\n");
      }
      else if ( token[1] == NULL)
      {
         printf("Error: Please use right format.\n");
      }
      else
      {
        unsigned char read_file[255];
        char  file_name[11];
        FILE *write_file;
        int i, j, size, flag = 0;

        fseek( filep, Dir, SEEK_SET );
        fread( dir, 16, sizeof( struct DirectoryEntry), filep );

        strcpy( file_name, token[1] );

        for ( j = 0; j < 16; j++ )
        {

          char * input = (char*) malloc( strlen(token[1])+1 );
          memset( input, 0, strlen( token[1] ) + 1 );
          strncpy( input, token[1], strlen( token[1] ) );

          if ( compare( dir[j].DIR_Name, input ) && dir[j].DIR_Attr == 0x20 )
          {

            int D = LBAToOffset(dir[j].DIR_FirstClusterLow);
            fseek( filep, D , SEEK_SET );

            size = dir[j].Dir_FileSize;

            if(token[2] == NULL)
            {
              write_file = fopen(file_name,"w");
            }
            else
            {
              // if the file name more than 100 characters
              // and if token[3] == NULL that mean no spaces in the file name
              if ( (strlen( token[2] ) <= 100) && (token[3] == NULL) )
              {
                write_file = fopen(token[2],"w");
              }
              else
              {
                printf("Error: fat32 image files shall not ");
                printf("contain spaces and shall be limited to 100 characters.\n");
                flag =1;
                break;
              }

            }
              // this loop is for when the file size is bigger than the
              //read_file than it can read smaller chunks and work efficiently
            while ( size >= 512 )
            {

              memset( read_file, 0, 256 );
              fread( read_file, 255, 1, filep );
              fwrite(read_file,1,255,write_file);

              size = size - 255;
            }

            fread( read_file, size, 1, filep );
            fwrite(read_file,1,size,write_file);

            fclose( write_file );
            flag =1;
          }
          free ( input );
        }

        if (flag == 0 )
        {
          printf("Error: File not found.\n");
        }

      }

    }
    else
    {
      if ( filep == NULL )
      {
        printf("Error: File system image must be opened first.\n");
      }
      else
      {
        printf("Error: Command not found.\n");
      }
    }


    free( working_root );

  }
  return 0;
}

int LBAToOffset (int32_t sector)
{
  return (( sector - 2) * BPB_BytsPerSec) + (BPB_BytsPerSec * BPB_RsvdSecCnt) +
         (BPB_NumFATS * BPB_FATSz32 * BPB_BytsPerSec);
}

// this function compares uppercase and lowercase chars to check if they the same
int compare (char *DIR_Name, char *input )
{
  char IMG_Name[12];

  strncpy( IMG_Name, DIR_Name, 11 );
  IMG_Name[11] = '\0';
  int com  = 0;
  char expanded_name[12];
  memset( expanded_name, ' ', 12 );

  char *token = strtok( input, "." );

  strncpy( expanded_name, token, strlen( token ) );

  token = strtok( NULL, "." );

  if( token )
  {
    strncpy( (char*)(expanded_name+8), token, strlen(token ) );
  }

  expanded_name[11] = '\0';

  int i;
  for( i = 0; i < 11; i++ )
  {
    expanded_name[i] = toupper( expanded_name[i] );
  }

  if( strncmp( expanded_name, IMG_Name, 11 ) == 0 )
  {
    com = 1;
  }

  return com;
}
