/*
Luca Fossati (Luca.Fossati@esa.int), European Space Agency

Software distributed under the "European Space Agency Public License – v2.0".

All Distribution of the Software and/or Modifications, as Source Code or Object Code,
must be, as a whole, under the terms of the European Space Agency Public License – v2.0.
If You Distribute the Software and/or Modifications as Object Code, You must:
(a)	provide in addition a copy of the Source Code of the Software and/or
Modifications to each recipient; or
(b)	make the Source Code of the Software and/or Modifications freely accessible by reasonable
means for anyone who possesses the Object Code or received the Software and/or Modifications
from You, and inform recipients how to obtain a copy of the Source Code.

The Software is provided to You on an “as is” basis and without warranties of any
kind, including without limitation merchantability, fitness for a particular purpose,
absence of defects or errors, accuracy or non-infringement of intellectual property
rights.
Except as expressly set forth in the "European Space Agency Public License – v2.0",
neither Licensor nor any Contributor shall be liable, including, without limitation, for direct, indirect,
incidental, or consequential damages (including without limitation loss of profit),
however caused and on any theory of liability, arising in any way out of the use or
Distribution of the Software or the exercise of any rights under this License, even
if You have been advised of the possibility of such damages.

*****************************************************************************************
Converts images from BSQ/BI format to BSQ/BI; the output file can either be a binary file
or a textual file.
*/

/*
USAGE
converter --input residuals --output converted --rows num_rows --columns num_col --bands num_bands \
--in_format [BSQ|BI] --in_depth num --in_byte_ordering [LITTLE|big] --dyn_range num --out_format [BSQ|bi] \
--out_depth num --text

*/

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#ifndef WIN32
#include <getopt.h>
#else
#include "win32_getopt.h"
#endif

#include "entropy_encoder.h"
#include "utils.h"
#include "predictor.h"

//String specifying the program command line
#define USAGE_STRING "Usage: %s --input residuals --output converted --rows num_rows --columns num_col --bands num_bands \
--in_format [BSQ|BI] --in_depth num --in_byte_ordering [LITTLE|big] --dyn_range num --out_format [BSQ|bi] \
--out_depth num --text\n"

//Lets declare the available program options: lets start
//with their long description...
struct option options[] = {
    {"input", 1, NULL, 1},
    {"output", 1, NULL, 2},
    {"rows", 1, NULL, 3},
    {"columns", 1, NULL, 4},
    {"bands", 1, NULL, 5},
    {"in_format", 1, NULL, 6},
    {"in_depth", 1, NULL, 7},
    {"in_byte_ordering", 1, NULL, 8},
    {"out_format", 1, NULL, 9},
    {"out_depth", 1, NULL, 10},
    {"dyn_range", 1, NULL, 11},
    {"text", 0, NULL, 12},
    {"help", 0, NULL, 13},
    {NULL, 0, NULL, 0}
};

//Lets finally start with the main program; actually it does nothing but
//parsing the options and calling the appropriate functions which do the
//actual job
int main(int argc, char *argv[]){
    //Configuration and command line variables
    char samples_file[128];
    char out_file[128];
    input_feature_t input_params;
    encoder_config_t encoder_params;
    int foundOpt = 0;
    int textual_out = 0;
    unsigned short int * samples = NULL;
    FILE * converted_file = NULL;

    // Some initialization of default values
    samples_file[0] = '\x0';
    out_file[0] = '\x0';
    memset(&input_params, 0, sizeof(input_feature_t));
    memset(&encoder_params, 0, sizeof(encoder_config_t));
    input_params.dyn_range = 16;

    //Lets do some simple command line option parsing
    do{
        foundOpt = getopt_long(argc, argv, "", options, NULL);
        switch(foundOpt){
            case 1:
                strcpy(samples_file, optarg);
            break;
            case 2:
                strcpy(out_file, optarg);
            break;
            case 3:
                input_params.y_size = (unsigned int)atoi(optarg);
            break;
            case 4:
                input_params.x_size = (unsigned int)atoi(optarg);
            break;
            case 5:
                input_params.z_size = (unsigned int)atoi(optarg);
            break;
            case 6:
                if(strcmp(optarg, "BI") == 0 || strcmp(optarg, "bi") == 0){
                    input_params.in_interleaving = BI;
                }
                else if(strcmp(optarg, "BSQ") == 0 || strcmp(optarg, "bsq") == 0){
                    input_params.in_interleaving = BSQ;
                }
                else{
                    fprintf(stderr, "\nError, %s unknown input image format\n\n", optarg);
                    fprintf(stderr, USAGE_STRING, argv[0]);
                    return -1;
                }
            break;
            case 7:
                input_params.in_interleaving_depth = (unsigned int)atoi(optarg);
            break;
            case 8:
                if(strcmp(optarg, "little") == 0 || strcmp(optarg, "LITTLE") == 0){
                    input_params.byte_ordering = LITTLE;
                }
                else if(strcmp(optarg, "big") == 0 || strcmp(optarg, "BIG") == 0){
                    input_params.byte_ordering = BIG;
                }
                else{
                    fprintf(stderr, "\nError, %s unknown input byte ordering\n\n", optarg);
                    fprintf(stderr, USAGE_STRING, argv[0]);
                    return -1;
                }
            break;
            case 9:
                if(strcmp(optarg, "BI") == 0 || strcmp(optarg, "bi") == 0){
                    encoder_params.out_interleaving = BI;
                }
                else if(strcmp(optarg, "BSQ") == 0 || strcmp(optarg, "bsq") == 0){
                    encoder_params.out_interleaving = BSQ;
                }
                else{
                    fprintf(stderr, "\nError, %s unknown image format\n\n", optarg);
                    fprintf(stderr, USAGE_STRING, argv[0]);
                    return -1;
                }
            break;
            case 10:
                encoder_params.out_interleaving_depth = (unsigned int)atoi(optarg);
            break;
            case 11:
                input_params.dyn_range = (unsigned int)atoi(optarg);
            break;
            case 12:
                textual_out = 1;
            break;
            case 13:
                fprintf(stderr, USAGE_STRING, argv[0]);
                return 0;
            break;
            case -1:
                //Do nothing, we have finished parsing the options
            break;
            case '?':
            default:
                fprintf(stderr, "\nError in the program command line!!\n\n");
                fprintf(stderr, USAGE_STRING, argv[0]);
                return -1;
            break;
        }
    }while(foundOpt >= 0);

    ///Now we need to perform a few checks that the necessary options have been provided
    //and we can call the function to perform the encoding.
    if(samples_file[0] == '\x0'){
        fprintf(stderr, "\nError, please indicate the file containing the input samples to be compressed\n\n");
        fprintf(stderr, USAGE_STRING, argv[0]);
        return -1;
    }
    if(out_file[0] == '\x0'){
        fprintf(stderr, "\nError, please indicate the file where the compressed stream will be saved\n\n");
        fprintf(stderr, USAGE_STRING, argv[0]);
        return -1;
    }
    if(input_params.y_size*input_params.x_size*input_params.z_size == 0){
        fprintf(stderr, "\nError, please specify all the x, y, and z dimensions with a number > 0\n\n");
        fprintf(stderr, USAGE_STRING, argv[0]);
        return -1;
    }
    if(input_params.in_interleaving == BI && (input_params.in_interleaving_depth < 1 || input_params.in_interleaving_depth > input_params.z_size)){
        fprintf(stderr, "\nError, the input interleaving depth has to be a positive integer not bigger than the number of bands\n\n");
        fprintf(stderr, USAGE_STRING, argv[0]);
        return -1;
    }
    if(encoder_params.out_interleaving == BI && (encoder_params.out_interleaving_depth < 1 || encoder_params.out_interleaving_depth > input_params.z_size)){
        fprintf(stderr, "\nError, the output interleaving depth has to be a positive integer not bigger than the number of bands\n\n");
        fprintf(stderr, USAGE_STRING, argv[0]);
        return -1;
    }
    if(input_params.dyn_range < 2 || input_params.dyn_range > 16){
        fprintf(stderr, "\nError, please specify the bit width of the residuals between 2 and 16 bits\n\n");
        fprintf(stderr, USAGE_STRING, argv[0]);
        return -1;
    }

    // *********************** here is the actual conversion step
    samples = (unsigned short int *)malloc(sizeof(unsigned short int)*input_params.x_size*input_params.y_size*input_params.z_size);
    if(samples == NULL){
        fprintf(stderr, "Error in allocating %lf kBytes for the samples buffer\n\n", ((double)sizeof(unsigned short int)*input_params.x_size*input_params.y_size*input_params.z_size)/1024.0);
        return -1;
    }
    // Reading the input samples
    if(read_samples(input_params, samples_file, samples) != 0){
        fprintf(stderr, "Error in reading the input samples\n");
        return -1;
    }
    // printing to the output file
    if(textual_out == 0)
        converted_file = fopen(out_file, "w+b");
    else
        converted_file = fopen(out_file, "w+t");
    if(converted_file == NULL){
        fprintf(stderr, "\nError in creating the output file\n\n");
        return -1;            
    }
    if(encoder_params.out_interleaving == BSQ){
        unsigned int x = 0, y = 0, z = 0;
        for(z = 0; z < input_params.z_size; z++){
            for(y = 0; y < input_params.y_size; y++){
                for(x = 0; x < input_params.x_size; x++){
                    if(textual_out != 0){
                        fprintf(converted_file, "%#x ", MATRIX_BSQ_INDEX(samples, input_params, x, y, z));
                    }else{
                        fwrite(&(MATRIX_BSQ_INDEX(samples, input_params, x, y, z)), 2, 1, converted_file);
                    }
                }
                if(textual_out != 0){
                    fprintf(converted_file, "\n");
                }
            }
            if(textual_out != 0){
                fprintf(converted_file, "\n");
            }
        }
    }
    else{
        unsigned int x = 0, y = 0, z = 0, i = 0;
        unsigned int interleaving_counter = input_params.z_size/encoder_params.out_interleaving_depth;
        if((input_params.z_size % encoder_params.out_interleaving_depth) != 0){
            interleaving_counter++;
        }
        for(y = 0; y < input_params.y_size; y++){
            for(i = 0; i < interleaving_counter; i++){
                for(x = 0; x < input_params.x_size; x++){
                    for(z = i*encoder_params.out_interleaving_depth; z < MIN((i+1)*encoder_params.out_interleaving_depth, input_params.z_size); z++){
                        if(textual_out != 0){
                            fprintf(converted_file, "%#x ", MATRIX_BSQ_INDEX(samples, input_params, x, y, z));
                        }else{
                            fwrite(&(MATRIX_BSQ_INDEX(samples, input_params, x, y, z)), 2, 1, converted_file);
                        }
                    }
                    if(textual_out != 0){
                        fprintf(converted_file, "\n");
                    }
                }
            }
            if(textual_out != 0){
                fprintf(converted_file, "\n");
            }
        }
    }
    fclose(converted_file);

    return 0;
}

