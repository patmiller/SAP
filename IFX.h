#ifndef IFX_h
#define IFX_h

enum IFTypeCode {
  IFArray = 0,
  IFBasic = 1,
  IFField = 2,
  IFFunction = 3,
  IFMultiple = 4,
  IFRecord = 5,
  IFStream = 6,
  IFTag = 7,
  IFTuple = 8,
  IFUnion = 9,
  IFWild = 10
};

enum IFBasics {
  IFBoolean = 0,
  IFCharacter = 1,
  IFDoubleReal = 2,
  IFInteger = 3,
  IFNull = 4,
  IFReal = 5,
  IFWildBasic = 6
};

#endif
