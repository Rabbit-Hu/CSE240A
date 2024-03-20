//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Ada Yuan";
const char *studentID = "A59024350";
const char *email = "x9yuan@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

int perceptronTheta;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//

uint32_t global_history;

// Gshare Predictor
uint8_t *gshare_predictor_table;

// Tournament Predictor
uint8_t *local_predictor_table;
uint32_t *local_history_table;
uint8_t *global_predictor_table;
uint8_t *choice_predictor_table;

// Custom Predictor (Perceptron)
int8_t **weights_table;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

void gshare_init()
{
  // Initialize the gshare predictor
  gshare_predictor_table = malloc(sizeof(uint8_t) * (1 << ghistoryBits));
  memset(gshare_predictor_table, WN, sizeof(uint8_t) * (1 << ghistoryBits));

  global_history = 0;
}

void tournament_init()
{
  // Initialize the tournament predictor
  local_predictor_table = malloc(sizeof(uint8_t) * (1 << lhistoryBits));
  local_history_table = malloc(sizeof(uint32_t) * (1 << pcIndexBits));
  global_predictor_table = malloc(sizeof(uint8_t) * (1 << ghistoryBits));
  choice_predictor_table = malloc(sizeof(uint8_t) * (1 << ghistoryBits));

  memset(local_predictor_table, WN, sizeof(uint8_t) * (1 << lhistoryBits));
  memset(local_history_table, 0, sizeof(uint32_t) * (1 << pcIndexBits));
  memset(global_predictor_table, WN, sizeof(uint8_t) * (1 << ghistoryBits));
  memset(choice_predictor_table, WT, sizeof(uint8_t) * (1 << ghistoryBits)); // start with weakly global

  global_history = 0;
}

void custom_init()
{
  // Initialize the custom predictor (preceptron)

  // weights = weights_table[pc_index][global_history_bit]
  // size: (1 << pcIndexBits) * ghistoryBits * 8 bits

  weights_table = malloc(sizeof(int8_t *) * (1 << pcIndexBits));
  for (int i = 0; i < (1 << pcIndexBits); i++)
  {
    weights_table[i] = malloc(sizeof(int8_t) * ghistoryBits);
    memset(weights_table[i], 0, sizeof(int8_t) * ghistoryBits);
  }

  global_history = 0;

  printf("[custom_init] ghistoryBits: %d, pcIndexBits: %d, perceptronTheta: %d\n", ghistoryBits, pcIndexBits, perceptronTheta);
  printf("[custom_init] Size of weights_table: %d K bits\n", (1 << pcIndexBits) * ghistoryBits * 8 / 1024);
}

// Initialize the predictor
//
void init_predictor()
{
  //
  // TODO: Initialize Branch Predictor Data Structures
  //
  global_history = 0;

  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    gshare_init();
    break;
  case TOURNAMENT:
    tournament_init();
    break;
  case CUSTOM:
    custom_init();
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//

uint8_t
gshare_make_prediction(uint32_t pc)
{
  // Rederence: CSE240A Lecture 8 slides, Page 8

  uint32_t mask = (1 << ghistoryBits) - 1;
  uint32_t index = (pc & mask) ^ global_history;

  return gshare_predictor_table[index] >= WT;
}

uint8_t
tournament_make_prediction(uint32_t pc)
{
  // Reference: https://acg.cis.upenn.edu/milom/cis501-Fall09/papers/Alpha21264.pdf
  // See Figure 4 on Page 28

  uint32_t local_index = pc & ((1 << pcIndexBits) - 1);
  uint32_t local_history = local_history_table[local_index];
  uint8_t local_prediction = local_predictor_table[local_history] >= WT;

  uint8_t global_prediction = global_predictor_table[global_history] >= WT;
  uint8_t choice_prediction = choice_predictor_table[global_history] >= WT;

  return choice_prediction == TAKEN ? global_prediction : local_prediction;
  // taken means global prediction, not taken means local prediction
}

uint8_t
custom_make_prediction(uint32_t pc)
{
  // Implement the custom predictor (perceptron)

  // hash table indexing is similar to gshare
  uint32_t mask = (1 << pcIndexBits) - 1;
  uint32_t index = (pc ^ global_history) & mask;
  int8_t *weights = weights_table[index];

  int sum = 0;
  for (int i = 0; i < ghistoryBits; i++)
    sum += weights[i] * (((global_history >> i) & 1) ? 1 : -1);

  return sum >= 0;
}

uint8_t
make_prediction(uint32_t pc)
{
  //
  // TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_make_prediction(pc);
  case TOURNAMENT:
    return tournament_make_prediction(pc);
  case CUSTOM:
    return custom_make_prediction(pc);
  default:
    break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void gshare_train_predictor(uint32_t pc, uint8_t outcome)
{
  // Update the gshare predictor
  uint32_t mask = (1 << ghistoryBits) - 1;
  uint32_t index = (pc & mask) ^ global_history;

  if (outcome == TAKEN && gshare_predictor_table[index] != ST)
    gshare_predictor_table[index]++;
  else if (outcome == NOTTAKEN && gshare_predictor_table[index] != SN)
    gshare_predictor_table[index]--;

  global_history = ((global_history << 1) | outcome) & mask;
}

void tournament_train_predictor(uint32_t pc, uint8_t outcome)
{
  uint32_t local_index = pc & ((1 << pcIndexBits) - 1);
  uint32_t local_history = local_history_table[local_index];
  uint8_t local_prediction = local_predictor_table[local_history] >= WT;

  uint8_t global_prediction = global_predictor_table[global_history] >= WT;
  uint8_t choice_prediction = choice_predictor_table[global_history] >= WT;

  if (local_prediction != outcome && global_prediction == outcome)
  { // global prediction is correct, local prediction is wrong
    if (choice_predictor_table[global_history] != ST)
      choice_predictor_table[global_history]++; // lean towards global
  }
  else if (local_prediction == outcome && global_prediction != outcome)
  { // local prediction is correct, global prediction is wrong
    if (choice_predictor_table[global_history] != SN)
      choice_predictor_table[global_history]--; // lean towards local
  }

  if (outcome == TAKEN)
  {
    if (local_predictor_table[local_history] != ST)
      local_predictor_table[local_history]++;
    if (global_predictor_table[global_history] != ST)
      global_predictor_table[global_history]++;
  }
  else
  {
    if (local_predictor_table[local_history] != SN)
      local_predictor_table[local_history]--;
    if (global_predictor_table[global_history] != SN)
      global_predictor_table[global_history]--;
  }

  local_history_table[local_index] = ((local_history << 1) | outcome) & ((1 << lhistoryBits) - 1);
  global_history = ((global_history << 1) | outcome) & ((1 << ghistoryBits) - 1);
}

int update_weight(int8_t weight, int8_t update)
{
  int max_weight = 127;
  int min_weight = -128;
  if (weight + update > max_weight)
    return max_weight;
  if (weight + update < min_weight)
    return min_weight;
  return weight + update;
}

void custom_train_predictor(uint32_t pc, uint8_t outcome)
{
  // Implement the custom predictor (perceptron)

  uint32_t mask = (1 << pcIndexBits) - 1;
  uint32_t index = (pc ^ global_history) & mask;
  int8_t *weights = weights_table[index];

  int sum = 0;
  for (int i = 0; i < ghistoryBits; i++)
    sum += weights[i] * (((global_history >> i) & 1) ? 1 : -1);

  uint8_t prediction = sum >= 0;

  // if (outcome == TAKEN && sum <= target_gap)
  //   for (int i = 0; i < ghistoryBits; i++)
  //     weights[i] += ((global_history >> i) & 1) ? 1 : -1;
  // else if (outcome == NOTTAKEN && sum >= -target_gap)
  //   for (int i = 0; i < ghistoryBits; i++)
  //     weights[i] -= ((global_history >> i) & 1) ? 1 : -1;

  if (outcome != prediction || (sum > -perceptronTheta && sum < perceptronTheta))
  {
    int8_t sign_error = outcome == TAKEN ? 1 : -1;
    for (int i = 0; i < ghistoryBits; i++)
      weights[i] = update_weight(weights[i], sign_error * (((global_history >> i) & 1) ? 1 : -1));
  }

  global_history = (global_history << 1) | outcome;
}

void train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  // TODO: Implement Predictor training
  //

  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    gshare_train_predictor(pc, outcome);
    break;
  case TOURNAMENT:
    tournament_train_predictor(pc, outcome);
    break;
  case CUSTOM:
    custom_train_predictor(pc, outcome);
    break;
  default:
    break;
  }
}
