#include "Scales.hpp"

const ScaleDefinition ScaleTable::scaleTable[NUM_SCALES] = {
    { "Major",  7, {  0,  2,  4,  5,  7,  9, 11, } },
    { "Minor",  7, {  0,  2,  3,  5,  7,  8, 10, } },
    { "Harm Minor",  7, {  0,  2,  3,  5,  7,  8, 11, } },
    { "Mel Minor",  7, {  0,  2,  3,  5,  7,  9, 11, } },
    { "Pent Major",  5, {  0,  2,  4,  7,  9, } },
    { "Pent Minor",  5, {  0,  3,  5,  7, 10, } },
    { "Ionian",  7, {  0,  2,  4,  5,  7,  9, 11, } },
    { "Dorian",  7, {  0,  2,  3,  5,  7,  9, 10, } },
    { "Prygian",  7, {  0,  1,  3,  5,  7,  8, 10, } },
    { "Lydian",  7, {  0,  2,  4,  6,  7,  9, 11, } },
    { "Mixolydian",  7, {  0,  2,  4,  5,  7,  9, 10, } },
    { "Aeolian",  7, {  0,  2,  3,  5,  7,  8, 10, } },
    { "Locrian",  7, {  0,  1,  3,  5,  6,  8, 10, } },
    { "Aegean",  5, {  0,  4,  6,  7, 11, } },
    { "Akebono",  5, {  0,  2,  3,  7,  8, } },
    { "Arboreal",  5, {  0,  1,  3,  5,  8, } },
    { "Avalon",  6, {  0,  3,  5,  7,  8, 10, } },
    { "Awake",  5, {  0,  3,  7,  8, 10, } },
    { "Blackpool",  6, {  0,  1,  3,  5,  6,  7, } },
    { "Blues",  6, {  0,  3,  5,  8, 10, 11, } },
    { "Caspian",  6, {  0,  2,  3,  6,  7, 11, } },
    { "Celtic Minor",  6, {  0,  2,  3,  5,  7, 10, } },
    { "Chad Gayo",  5, {  0,  5,  7,  8, 10, } },
    { "Daxala",  7, {  0,  2,  3,  6,  7,  8, 11, } },
    { "Deshvara",  5, {  0,  2,  5,  7, 11, } },
    { "Ebbtide",  6, {  0,  1,  3,  7,  8, 10, } },
    { "Elysian",  6, {  0,  2,  4,  7,  9, 11, } },
    { "Emmanuel",  6, {  0,  1,  4,  5,  7,  8, } },
    { "Fifth Mode",  6, {  0,  3,  5,  7,  9, 10, } },
    { "Golden Dawn",  5, {  0,  2,  6,  7, 11, } },
    { "Golden Gate",  6, {  0,  2,  4,  6,  7, 11, } },
    { "Goonkali",  5, {  0,  1,  5,  6, 10, } },
    { "Gowleeswari",  4, {  0,  3,  7,  8, } },
    { "Hafiz",  6, {  0,  3,  5,  7,  8, 11, } },
    { "Hijaz",  7, {  0,  1,  3,  5,  6,  9, 10, } },
    { "Hijazkiar",  7, {  0,  1,  4,  5,  6,  9, 10, } },
    { "Hokkaido",  6, {  0,  1,  5,  7,  8, 10, } },
    { "Hungarian major",  7, {  0,  3,  5,  6,  8,  9, 11, } },
    { "Huzam",  7, {  0,  2,  4,  5,  8,  9, 10, } },
    { "Insen",  6, {  0,  2,  3,  5,  7,  8, } },
    { "Integral",  6, {  0,  2,  3,  7,  8, 10, } },
    { "Inuit",  4, {  0,  5,  7,  9, } },
    { "Iwato",  5, {  0,  1,  5,  7,  8, } },
    { "Kambhoji",  6, {  0,  2,  5,  7,  9, 10, } },
    { "Kapijingla",  6, {  0,  4,  5,  7,  9, 10, } },
    { "Kedaram",  6, {  0,  2,  6,  7,  9, 11, } },
    { "Khyberi",  6, {  0,  2,  3,  6,  7, 10, } },
    { "Kokin-Choshi",  5, {  0,  3,  5,  6, 10, } },
    { "Kumari",  6, {  0,  3,  4,  7,  9, 11, } },
    { "Kumo",  6, {  0,  1,  3,  5,  7,  8, } },
    { "La Sirena",  6, {  0,  2,  3,  7,  9, 10, } },
    { "Limoncello",  6, {  0,  2,  4,  5,  7, 11, } },
    { "Magic Hour",  5, {  0,  2,  3,  7, 10, } },
    { "Melog",  5, {  0,  4,  5,  7, 11, } },
    { "Mixophonic",  6, {  0,  2,  4,  5,  7, 10, } },
    { "MonDhra",  7, {  0,  1,  4,  5,  7,  8, 10, } },
    { "Mysorean",  7, {  0,  2,  3,  4,  5,  7, 11, } },
    { "Noh",  7, {  0,  1,  2,  4,  5,  7, 10, } },
    { "Olympos",  6, {  0,  3,  5,  6, 10, 11, } },
    { "Onoleo",  5, {  0,  4,  5,  7,  8, } },
    { "Oxalis",  5, {  0,  4,  5,  7,  9, } },
    { "Oxalista",  6, {  0,  2,  4,  5,  7,  9, } },
    { "Paradiso",  5, {  0,  2,  4,  7, 11, } },
    { "Purvi",  7, {  0,  3,  4,  5,  8, 10, 11, } },
    { "Pygmy",  5, {  0,  3,  5,  7,  8, } },
    { "Pyramid",  6, {  0,  1,  4,  7,  9, 10, } },
    { "Raga Desh",  5, {  0,  4,  5,  7, 10, } },
    { "Russian Major",  6, {  0,  4,  5,  7,  9, 11, } },
    { "SalaDin",  6, {  0,  1,  4,  5,  7, 10, } },
    { "Saudade",  6, {  0,  2,  3,  5,  7, 11, } },
    { "Sen",  5, {  0,  1,  5,  7, 10, } },
    { "Shakti",  6, {  0,  2,  3,  6,  7,  9, } },
    { "Shanti",  6, {  0,  2,  4,  6,  7,  9, } },
    { "Syrah",  6, {  0,  2,  3,  7,  8, 11, } },
    { "Tharsi",  7, {  0,  2,  3,  5,  6,  7, 11, } },
    { "Ujo",  5, {  0,  2,  5,  7, 10, } },
    { "Voyager",  5, {  0,  4,  7,  9, 11, } },
    { "Wadi Rum",  6, {  0,  3,  6,  7,  8, 11, } },
    { "Zokuso",  5, {  0,  1,  3,  7,  8, } },
};

const ScaleDefinition * ScaleTable::findScaleByName(std::string name) {
    for (int i = 0; i < ScaleTable::NUM_SCALES; i++) {
        if (scaleTable[i].name == name) {
            return &scaleTable[i];
        }
    }
    return 0; // no match
}