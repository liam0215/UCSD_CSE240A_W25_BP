//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <cstdint>
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Liam Arzola";
const char *studentID = "A69031722";
const char *email = "larzola@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = {"Static", "Gshare",
                         "Tournament", "Custom"};

// define number of bits required for indexing the BHT here.
int ghistoryBits = 17; // Number of bits used for Global History
int path_history_bits = 15; // Number of bits used for Path History
int chooser_bits = 15; // Number of bits used for Chooser
int lhistoryBits = 15; // Number of bits used for Local History
int pcIndexBits = 12;  // Number of bits used for PC index
int bpType;            // Branch Prediction Type
int verbose;

int custom_path_history_bits = 15; // Number of bits used for Path History
int custom_chooser_bits = 15; // Number of bits used for Chooser
int custom_lhistoryBits = 15; // Number of bits used for Local History
int custom_pcIndexBits = 12;  // Number of bits used for PC index

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
// TODO: Add your own Branch Predictor data structures here
//
// gshare
uint8_t *bht_gshare;
uint64_t ghistory;

// tournament
uint8_t *bht_tournament_local;
uint32_t *lht_tournament_local;
uint8_t *bht_chooser_tournament;
uint8_t *bht_tournament_global;
uint64_t path_history;

// custom
uint64_t custom_path_history;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

// gshare functions
void init_gshare()
{
  int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t *)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for (i = 0; i < bht_entries; i++)
  {
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

uint8_t gshare_predict(uint32_t pc)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch (bht_gshare[index])
  {
  case WN:
    return NOTTAKEN;
  case SN:
    return NOTTAKEN;
  case WT:
    return TAKEN;
  case ST:
    return TAKEN;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    return NOTTAKEN;
  }
}

void train_gshare(uint32_t pc, uint8_t outcome)
{
  // get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries - 1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries - 1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  // Update state of entry in bht based on outcome
  switch (bht_gshare[index])
  {
  case WN:
    bht_gshare[index] = (outcome == TAKEN) ? WT : SN;
    break;
  case SN:
    bht_gshare[index] = (outcome == TAKEN) ? WN : SN;
    break;
  case WT:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WN;
    break;
  case ST:
    bht_gshare[index] = (outcome == TAKEN) ? ST : WT;
    break;
  default:
    printf("Warning: Undefined state of entry in GSHARE BHT!\n");
    break;
  }

  // Update history register
  ghistory = ((ghistory << 1) | outcome);
}

void cleanup_gshare()
{
  free(bht_gshare);
}

// tournament functions
void init_tournament() {
  int bht_entries_local = 1 << lhistoryBits;
  int bht_entries_global = 1 << path_history_bits;
  int lht_entries = 1 << pcIndexBits;
  int chooser_entries = 1 << chooser_bits;
  bht_tournament_local = (uint8_t *)calloc(bht_entries_local, sizeof(uint8_t));
  lht_tournament_local = (uint32_t *)calloc(lht_entries, sizeof(uint32_t));
  bht_chooser_tournament = (uint8_t *)calloc(chooser_entries, sizeof(uint8_t));
  bht_tournament_global = (uint8_t *)calloc(bht_entries_global, sizeof(uint8_t));
  for (int i = 0; i < bht_entries_local; i++)
  {
    bht_tournament_local[i] = WN;
  }
  for (int i = 0; i < lht_entries; i++)
  {
    lht_tournament_local[i] = 0;
  }
  for (int i = 0; i < bht_entries_global; i++)
  {
    bht_chooser_tournament[i] = WN;
    bht_tournament_global[i] = WN;
  }
  path_history = 0;
}

uint8_t tournament_predict(uint32_t pc)
{
  int bht_entries_local = 1 << lhistoryBits;
  int bht_entries_global = 1 << path_history_bits;
  int lht_entries = 1 << pcIndexBits;
  int chooser_entries = 1 << chooser_bits;
  uint32_t pc_lower_bits = pc & (lht_entries - 1);
  uint32_t path_history_lower_bits = path_history & (bht_entries_global - 1);
  uint32_t chooser_index = path_history & (chooser_entries - 1);
  uint8_t chooser_prediction = bht_chooser_tournament[chooser_index];
  if (chooser_prediction == ST || chooser_prediction == WT) {
    uint32_t lht_entry = lht_tournament_local[pc_lower_bits] & (bht_entries_local - 1);
    uint8_t prediction = bht_tournament_local[lht_entry];
    if (prediction == ST || prediction == WT)
      return TAKEN;
    else if (prediction == SN || prediction == WN)
      return NOTTAKEN;
    else {
      printf("Error: Undefined prediction value!\n");
      exit(EXIT_FAILURE);
    }
  } else if (chooser_prediction == SN || chooser_prediction == WN) {
    uint8_t prediction = bht_tournament_global[path_history_lower_bits];
    if (prediction == ST || prediction == WT)
      return TAKEN;
    else if (prediction == SN || prediction == WN)
      return NOTTAKEN;
    else {
      printf("Error: Undefined prediction value!\n");
      exit(EXIT_FAILURE);
    }
  } else {
    printf("Error: Undefined chooser prediction value!\n");
    exit(EXIT_FAILURE);
  }
}

void train_tournament(uint32_t pc, uint8_t outcome)
{
  int bht_entries_local = 1 << lhistoryBits;
  int bht_entries_global = 1 << path_history_bits;
  int lht_entries = 1 << pcIndexBits;
  int chooser_entries = 1 << chooser_bits;
  uint32_t pc_lower_bits = pc & (lht_entries - 1);
  uint32_t path_history_lower_bits = path_history & (bht_entries_global - 1);
  uint32_t chooser_index = path_history & (chooser_entries - 1);
  uint8_t chooser_prediction = bht_chooser_tournament[chooser_index];

  uint32_t lht_entry = lht_tournament_local[pc_lower_bits] & (bht_entries_local - 1);
  uint8_t local_prediction = bht_tournament_local[lht_entry];
  uint8_t global_prediction = bht_tournament_global[path_history_lower_bits];
  
  uint8_t local_prediction_dir = (local_prediction == ST || local_prediction == WT) ? TAKEN : NOTTAKEN;
  uint8_t global_prediction_dir = (global_prediction == ST || global_prediction == WT) ? TAKEN : NOTTAKEN;
  if (local_prediction_dir != global_prediction_dir) {
    if (chooser_prediction == ST)
      bht_chooser_tournament[path_history_lower_bits] = (outcome == local_prediction_dir) ? ST : WT;
    else if (chooser_prediction == WT)
      bht_chooser_tournament[path_history_lower_bits] = (outcome == local_prediction_dir) ? ST : WN;
    else if (chooser_prediction == WN)
      bht_chooser_tournament[path_history_lower_bits] = (outcome == local_prediction_dir) ? WT : SN;
    else if (chooser_prediction == SN)
      bht_chooser_tournament[path_history_lower_bits] = (outcome == local_prediction_dir) ? WN : SN;
    else {
      printf("Error: Undefined chooser prediction value!\n");
      exit(EXIT_FAILURE);
    }
  } 

  if (local_prediction == ST)
    bht_tournament_local[lht_entry] = (outcome == TAKEN) ? ST : WT;
  else if (local_prediction == WT)
    bht_tournament_local[lht_entry] = (outcome == TAKEN) ? ST : WN;
  else if (local_prediction == WN)
    bht_tournament_local[lht_entry] = (outcome == TAKEN) ? WT : SN;
  else if (local_prediction == SN)
    bht_tournament_local[lht_entry] = (outcome == TAKEN) ? WN : SN;
  else {
    printf("Error: Undefined prediction value!\n");
    exit(EXIT_FAILURE);
  }

  if (global_prediction == ST)
    bht_tournament_global[path_history_lower_bits] = (outcome == TAKEN) ? ST : WT;
  else if (global_prediction == WT)
    bht_tournament_global[path_history_lower_bits] = (outcome == TAKEN) ? ST : WN;
  else if (global_prediction == WN)
    bht_tournament_global[path_history_lower_bits] = (outcome == TAKEN) ? WT : SN;
  else if (global_prediction == SN)
    bht_tournament_global[path_history_lower_bits] = (outcome == TAKEN) ? WN : SN;
  else {
    printf("Error: Undefined prediction value!\n");
    exit(EXIT_FAILURE);
  }

  path_history = ((path_history << 1) | outcome);
  lht_tournament_local[pc_lower_bits] = ((lht_entry << 1) | outcome);
}

// custom functions
void init_custom() {
  int bht_entries_local = 1 << custom_lhistoryBits;
  int bht_entries_global = 1 << custom_path_history_bits;
  int lht_entries = 1 << custom_pcIndexBits;
  int chooser_entries = 1 << custom_chooser_bits;
  bht_tournament_local = (uint8_t *)calloc(bht_entries_local, sizeof(uint8_t));
  lht_tournament_local = (uint32_t *)calloc(lht_entries, sizeof(uint32_t));
  bht_chooser_tournament = (uint8_t *)calloc(chooser_entries, sizeof(uint8_t));
  bht_tournament_global = (uint8_t *)calloc(bht_entries_global, sizeof(uint8_t));
  for (int i = 0; i < bht_entries_local; i++)
  {
    bht_tournament_local[i] = WN;
  }
  for (int i = 0; i < lht_entries; i++)
  {
    lht_tournament_local[i] = 0;
  }
  for (int i = 0; i < bht_entries_global; i++)
  {
    bht_chooser_tournament[i] = WN;
    bht_tournament_global[i] = WN;
  }
  custom_path_history = 0;
}

uint8_t custom_predict(uint32_t pc)
{
  int bht_entries_local = 1 << custom_lhistoryBits;
  int bht_entries_global = 1 << custom_path_history_bits;
  int lht_entries = 1 << custom_pcIndexBits;
  int chooser_entries = 1 << custom_chooser_bits;
  uint32_t pc_lower_bits = pc & (lht_entries - 1);
  uint32_t global_pc_lower_bits = pc & (bht_entries_global - 1);
  uint32_t path_history_lower_bits = custom_path_history & (bht_entries_global - 1);
  uint32_t path_history_folded = path_history_lower_bits;
  for (int i = 1; i < 64 / custom_path_history_bits; i++) {
    path_history_folded ^= (custom_path_history >> (i * custom_path_history_bits)) & (bht_entries_global - 1);
  }

  uint32_t chooser_path_history_folded = path_history_lower_bits;
  for (int i = 1; i < 64 / custom_chooser_bits; i++) {
    chooser_path_history_folded ^= (custom_path_history >> (i * custom_chooser_bits)) & (chooser_entries - 1);
  }
  uint32_t chooser_index = chooser_path_history_folded & (chooser_entries - 1);
  uint8_t chooser_prediction = bht_chooser_tournament[chooser_index];
  if (chooser_prediction == ST || chooser_prediction == WT) {
    uint32_t lht_entry = lht_tournament_local[pc_lower_bits] & (bht_entries_local - 1);
    uint8_t prediction = bht_tournament_local[lht_entry];
    if (prediction == ST || prediction == WT)
      return TAKEN;
    else if (prediction == SN || prediction == WN)
      return NOTTAKEN;
    else {
      printf("Error: Undefined prediction value!\n");
      exit(EXIT_FAILURE);
    }
  } else if (chooser_prediction == SN || chooser_prediction == WN) {
    uint8_t prediction = bht_tournament_global[global_pc_lower_bits ^ path_history_folded];
    if (prediction == ST || prediction == WT)
      return TAKEN;
    else if (prediction == SN || prediction == WN)
      return NOTTAKEN;
    else {
      printf("Error: Undefined prediction value!\n");
      exit(EXIT_FAILURE);
    }
  } else {
    printf("Error: Undefined chooser prediction value!\n");
    exit(EXIT_FAILURE);
  }
}

void train_custom(uint32_t pc, uint8_t outcome)
{
  int bht_entries_local = 1 << custom_lhistoryBits;
  int bht_entries_global = 1 << custom_path_history_bits;
  int lht_entries = 1 << custom_pcIndexBits;
  int chooser_entries = 1 << custom_chooser_bits;
  uint32_t pc_lower_bits = pc & (lht_entries - 1);
  uint32_t global_pc_lower_bits = pc & (bht_entries_global - 1);
  uint32_t path_history_lower_bits = custom_path_history & (bht_entries_global - 1);
  uint32_t path_history_folded = path_history_lower_bits;
  for (int i = 1; i < 64 / custom_path_history_bits; i++) {
    path_history_folded ^= (custom_path_history >> (i * custom_path_history_bits)) & (bht_entries_global - 1);
  }

  uint32_t chooser_path_history_folded = path_history_lower_bits;
  for (int i = 1; i < 64 / custom_chooser_bits; i++) {
    chooser_path_history_folded ^= (custom_path_history >> (i * custom_chooser_bits)) & (chooser_entries - 1);
  }
  uint32_t chooser_index = chooser_path_history_folded & (chooser_entries - 1);
  uint8_t chooser_prediction = bht_chooser_tournament[chooser_index];

  uint32_t lht_entry = lht_tournament_local[pc_lower_bits] & (bht_entries_local - 1);
  uint8_t local_prediction = bht_tournament_local[lht_entry];
  uint8_t global_prediction = bht_tournament_global[global_pc_lower_bits ^ path_history_folded];
  
  uint8_t local_prediction_dir = (local_prediction == ST || local_prediction == WT) ? TAKEN : NOTTAKEN;
  uint8_t global_prediction_dir = (global_prediction == ST || global_prediction == WT) ? TAKEN : NOTTAKEN;
  if (local_prediction_dir != global_prediction_dir) {
    if (chooser_prediction == ST)
      bht_chooser_tournament[path_history_folded] = (outcome == local_prediction_dir) ? ST : WT;
    else if (chooser_prediction == WT)
      bht_chooser_tournament[path_history_folded] = (outcome == local_prediction_dir) ? ST : WN;
    else if (chooser_prediction == WN)
      bht_chooser_tournament[path_history_folded] = (outcome == local_prediction_dir) ? WT : SN;
    else if (chooser_prediction == SN)
      bht_chooser_tournament[path_history_folded] = (outcome == local_prediction_dir) ? WN : SN;
    else {
      printf("Error: Undefined chooser prediction value!\n");
      exit(EXIT_FAILURE);
    }
  } 

  if (local_prediction == ST)
    bht_tournament_local[lht_entry] = (outcome == TAKEN) ? ST : WT;
  else if (local_prediction == WT)
    bht_tournament_local[lht_entry] = (outcome == TAKEN) ? ST : WN;
  else if (local_prediction == WN)
    bht_tournament_local[lht_entry] = (outcome == TAKEN) ? WT : SN;
  else if (local_prediction == SN)
    bht_tournament_local[lht_entry] = (outcome == TAKEN) ? WN : SN;
  else {
    printf("Error: Undefined prediction value!\n");
    exit(EXIT_FAILURE);
  }

  if (global_prediction == ST)
    bht_tournament_global[global_pc_lower_bits ^ path_history_folded] = (outcome == TAKEN) ? ST : WT;
  else if (global_prediction == WT)
    bht_tournament_global[global_pc_lower_bits ^ path_history_folded] = (outcome == TAKEN) ? ST : WN;
  else if (global_prediction == WN)
    bht_tournament_global[global_pc_lower_bits ^ path_history_folded] = (outcome == TAKEN) ? WT : SN;
  else if (global_prediction == SN)
    bht_tournament_global[global_pc_lower_bits ^ path_history_folded] = (outcome == TAKEN) ? WN : SN;
  else {
    printf("Error: Undefined prediction value!\n");
    exit(EXIT_FAILURE);
  }

  custom_path_history = ((custom_path_history << 1) | outcome);
  lht_tournament_local[pc_lower_bits] = ((lht_entry << 1) | outcome);
}

void init_predictor()
{
  switch (bpType)
  {
  case STATIC:
    break;
  case GSHARE:
    init_gshare();
    break;
  case TOURNAMENT:
    init_tournament();
    break;
  case CUSTOM:
    init_custom();
    break;
  default:
    break;
  }
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint32_t make_prediction(uint32_t pc, uint32_t target, uint32_t direct)
{

  // Make a prediction based on the bpType
  switch (bpType)
  {
  case STATIC:
    return TAKEN;
  case GSHARE:
    return gshare_predict(pc);
  case TOURNAMENT:
    return tournament_predict(pc);
  case CUSTOM:
    return custom_predict(pc);
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

void train_predictor(uint32_t pc, uint32_t target, uint32_t outcome, uint32_t condition, uint32_t call, uint32_t ret, uint32_t direct)
{
  if (condition)
  {
    switch (bpType)
    {
    case STATIC:
      return;
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tournament(pc, outcome);
    case CUSTOM:
      return train_custom(pc, outcome);
    default:
      break;
    }
  }
}
