#include "parser.h"

bool initialize_literal_parser_table(literal_parser_state_t table[]) {
  // Initialize all entries to ERROR
  for(int i=0;i<NSTATES*256;++i) table[i] = ERROR;

  // ----------------------------------------------------------------------
  // In the start state, we figure out what kind of literal this "looks like"
  // ----------------------------------------------------------------------
  table[START*256+'n'] = NIL0;
  table[START*256+'N'] = NIL0;
  table[START*256+'t'] = TRUE0;
  table[START*256+'T'] = TRUE0;
  table[START*256+'f'] = FALSE0;
  table[START*256+'F'] = FALSE0;
  table[START*256+'0'] = INTEGER;
  table[START*256+'1'] = INTEGER;
  table[START*256+'2'] = INTEGER;
  table[START*256+'3'] = INTEGER;
  table[START*256+'4'] = INTEGER;
  table[START*256+'5'] = INTEGER;
  table[START*256+'6'] = INTEGER;
  table[START*256+'7'] = INTEGER;
  table[START*256+'8'] = INTEGER;
  table[START*256+'9'] = INTEGER;
  table[START*256+'+'] = INTEGER;
  table[START*256+'-'] = INTEGER;
  table[START*256+'\''] = CHAR_BODY;
  table[START*256+'"'] = STRING_BODY;

  // ----------------------------------------------------------------------
  // Booleans are true or false
  // ----------------------------------------------------------------------
  table[TRUE0*256+'r'] = TRUE1;
  table[TRUE0*256+'R'] = TRUE1;
  table[TRUE1*256+'u'] = TRUE2;
  table[TRUE1*256+'U'] = TRUE2;
  table[TRUE2*256+'e'] = TRUE3;
  table[TRUE2*256+'E'] = TRUE3;
  table[TRUE3*256+0] = BOOLEAN_LITERAL;

  table[FALSE0*256+'a'] = FALSE1;
  table[FALSE0*256+'A'] = FALSE1;
  table[FALSE1*256+'l'] = FALSE2;
  table[FALSE1*256+'L'] = FALSE2;
  table[FALSE2*256+'s'] = FALSE3;
  table[FALSE2*256+'S'] = FALSE3;
  table[FALSE3*256+'e'] = FALSE4;
  table[FALSE3*256+'E'] = FALSE4;
  table[FALSE4*256+0] = BOOLEAN_LITERAL;

  // ----------------------------------------------------------------------
  // Characters are single quote with an escape
  // ----------------------------------------------------------------------
  for(char c=' '; c < 127; ++c) {
    if (c == '\\') {
      table[CHAR_BODY*256+c] = CHAR_ESCAPE;
    } else if (c == '\'') {
      table[CHAR_BODY*256+c] = ERROR;
    } else {
      table[CHAR_BODY*256+c] = CHAR_END;
    }
  }

  for(char c=' '; c < 127; ++c) {
    table[CHAR_ESCAPE*256+c] = CHAR_END;
  }

  table[CHAR_END*256+'\''] = CHAR_NULL;
  table[CHAR_NULL*256+0] = CHAR_LITERAL;

  // ----------------------------------------------------------------------
  // DoubleReal
  // ----------------------------------------------------------------------      
  table[DOUBLEREAL_EXP_OPTSIGN*256+0] = DOUBLEREAL_LITERAL;
  table[DOUBLEREAL_EXP_OPTSIGN*256+'+'] = DOUBLEREAL_EXP_DIGIT;
  table[DOUBLEREAL_EXP_OPTSIGN*256+'-'] = DOUBLEREAL_EXP_DIGIT;
  for(char c='0'; c <= '9'; ++c) {
    table[DOUBLEREAL_EXP_OPTSIGN*256+c] = DOUBLEREAL_EXP;
  }

  for(char c='0'; c <= '9'; ++c) {
    table[DOUBLEREAL_EXP_DIGIT*256+c] = DOUBLEREAL_EXP;
  }

  table[DOUBLEREAL_EXP*256+0] = DOUBLEREAL_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[DOUBLEREAL_EXP*256+c] = DOUBLEREAL_EXP;
  }

  // ----------------------------------------------------------------------
  // Integer
  // ----------------------------------------------------------------------  
  table[INTEGER*256+0] = INTEGER_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[INTEGER*256+c] = INTEGER;
  }
  table[INTEGER*256+'.'] = REAL;
  table[INTEGER*256+'e'] = REAL_EXP_OPTSIGN;
  table[INTEGER*256+'E'] = REAL_EXP_OPTSIGN;
  table[INTEGER*256+'d'] = DOUBLEREAL_EXP_OPTSIGN;
  table[INTEGER*256+'D'] = DOUBLEREAL_EXP_OPTSIGN;

  // ----------------------------------------------------------------------
  // Null
  // ----------------------------------------------------------------------      
  table[NIL0*256+'i'] = NIL1;
  table[NIL0*256+'I'] = NIL1;
  table[NIL1*256+'l'] = NIL2;
  table[NIL1*256+'L'] = NIL2;
  table[NIL2*256+  0] = NULL_LITERAL;

  // ----------------------------------------------------------------------
  // Real
  // ----------------------------------------------------------------------      
  table[REAL*256+0] = REAL_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[REAL*256+c] = REAL;
  }
  table[REAL*256+'e'] = REAL_EXP_OPTSIGN;
  table[REAL*256+'E'] = REAL_EXP_OPTSIGN;
  table[REAL*256+'d'] = DOUBLEREAL_EXP_OPTSIGN;
  table[REAL*256+'D'] = DOUBLEREAL_EXP_OPTSIGN;
      
  table[REAL_EXP_OPTSIGN*256+0] = REAL_LITERAL;
  table[REAL_EXP_OPTSIGN*256+'+'] = REAL_EXP_DIGIT;
  table[REAL_EXP_OPTSIGN*256+'-'] = REAL_EXP_DIGIT;
  for(char c='0'; c <= '9'; ++c) {
    table[REAL_EXP_OPTSIGN*256+c] = REAL_EXP;
  }

  for(char c='0'; c <= '9'; ++c) {
    table[REAL_EXP_DIGIT*256+c] = REAL_EXP;
  }

  table[REAL_EXP*256+0] = REAL_LITERAL;
  for(char c='0'; c <= '9'; ++c) {
    table[REAL_EXP*256+c] = REAL_EXP;
  }

  // ----------------------------------------------------------------------
  // String
  // ----------------------------------------------------------------------
  for(char c=' '; c < 127; ++c) {
    if (c == '\\') {
      table[STRING_BODY*256+c] = STRING_ESCAPE;
    } else if (c == '"') {
      table[STRING_BODY*256+c] = STRING_END;
    } else {
      table[STRING_BODY*256+c] = STRING_BODY;
    }
  }
  for(char c=' '; c < 127; ++c) {
    table[STRING_ESCAPE*256+c] = STRING_BODY;
  }
  table[STRING_END*256+0] = STRING_LITERAL;

  return true;
}

literal_parser_state_t parse_literal(const char* p) {
  // Initialize our state machine
  static literal_parser_state_t table[NSTATES*256];
  static bool table_ready = false;
  if (!table_ready) table_ready = initialize_literal_parser_table(table);

  // Use the state machine to figure out the type
  literal_parser_state_t state = START;
  while(state > 0) {
    state = table[state*256+*p++];
  }

  return state;
}

