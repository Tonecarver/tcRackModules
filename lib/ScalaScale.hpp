#ifndef _tc_scala_scale_hpp_
#define _tc_scala_scale_hpp_

#include <iostream>
#include <fstream>
#include <string>

template <int SIZE>
struct ScalaScale {
    std::string description;   // may be blank/epty 
    int numPitches;            // not counting the root frequency 
    float cents[ SIZE ];       // cents from the starting note 
                               // the starting frequency is implied as 0.0 for whatever pitch is deemed to be the root 
                               // cents[0] is the cents for the _second_ note in the scale 

    void clear() { 
        description.erase();
        numPitches = 0;
        memset(cents, 0, sizeof(cents));
    }

    bool loadFomFile(std::string filepath);



// for converting cents to pitches 
// https://en.wikipedia.org/wiki/Cent_(music)
// https://en.wikipedia.org/wiki/Equal_temperament
// https://en.wikipedia.org/wiki/12_equal_temperament
// https://www.huygens-fokker.org/scala/scl_format.html

    float computeFrequecny(float rootFreq, float cents) {
        return rootFreq * pow(2.f, cents / 1200.f);
    }


};


// https://www.huygens-fokker.org/scala/scl_format.html
// comments start with !
//   first non-commented line is description
//   second non-commented line is number of notes
//   one note (cents delta) per line 
//     lines with '.' are cents 
//     lines without '.' are ratio
//        / separates numer and (optional) denom
//        / is optional if denom not specified
//        default denom to 1.0 if denom not specified 

template <int SIZE>
struct ScalaParser {
    bool haveDescription;
    bool haveNoteCount;
    int numNotesCollected;
    int errorCount; 
    std::string filepath;
    ScalaScale<SIZE> tempScale; // internal workspace to build up values while parsing

    ScalaParser() { 
    }

    bool importFromFile(const std::string & filepath) {

        this->filepath = filepath;

        errorCount = 0;
        haveDescription = false;
        haveNoteCount = false;
        numNotesCollected = 0;
        tempScale.clear();

        bool loaded = false;

        std::fstream infile;
        infile.open(filepath, std::fstream::in);
        if (infile.is_open()) {
            loaded = loadScale(infile);
            infile.close();
        }
        return loaded;
    }

    bool loadScale(std::fstream & infile) {
        std::string line;
        while (std::getline(infile, line)) {
            processLine(line);
        }

        if (numNotesCollected != tempScale.numPitches) {
            reportError("Note count mismatch");
        }

        return errorCount == 0;
    }

    void processLine(const std::string& line) {

        //DEBUG("  scala parser: line = '%s'", line.c_str());

        if (isComment(line)) {
            return; // ignore comments
        }

        std::string str = trim(line);

        if (! haveDescription) {
            tempScale.description = str;
            haveDescription = true;
            //DEBUG("  scala parser: description = '%s'", str.c_str());
        }
        else if (! haveNoteCount) {
            tempScale.numPitches = parseNumPitches(str, 0, SIZE); // count of 0 is allowed 
            haveNoteCount = true;
            //DEBUG("  scala parser: note count = %d", tempScale.numPitches);
        }
        else if (numNotesCollected < tempScale.numPitches) {
            float cents = parseNoteValue(str);
            tempScale.cents[numNotesCollected] = cents;
            numNotesCollected ++;
            //DEBUG("  scala parser: cents[%d] = %f", numNotesCollected, cents);
        }
        else {
            reportError("Invalid line: " + line);
        }
    }

    int parseNumPitches(std::string str, int minValue, int maxValue) {
        int numPitches = convertToInteger(str);
        if (numPitches < minValue || numPitches > maxValue) {
            reportError("Num pitches is out of range: " + str);
            numPitches = 0;
        }
        return numPitches;
    }

    float parseNoteValue(const std::string & str) {
        if (str.find('.') != std::string::npos) {
            return parseCents(str);
        }
        else { // if (str.contains('/'))
            return parseRatio(str);
        }
    }

    float parseCents(const std::string & str) {
        //DEBUG(" parseCents: %s", str.c_str());
        float cents = convertToFloat(str);
        return cents;
    }

    float parseRatio(const std::string & str) {

        std::string numerator_str;
        std::string denom_str;

        size_t pos = 0;
        pos = str.find('/');
        if (pos == std::string::npos) { // not found 
            numerator_str = str;
        } else {
            numerator_str = str.substr(0,pos);
            denom_str = str.substr(pos + 1, str.length());
        }

        float numer = convertToFloat(numerator_str);
        float denom = 1.0;
        if (denom_str.length() > 0) {
            denom = convertToFloat(denom_str);
            if (denom <= 0) {
                denom = 1.0;
            }
        }

        float ratio = numer / denom; 
        float cents = 1200 * log2(ratio);

        //DEBUG(" Ratio: numer %f, denom %f = ratio %f, cents %f", numer, denom, ratio, cents);

        return cents;

        // split at '/'
        // convert numerator to float 
        // convert denominator to float 
        // check denominator > 0
        // if denominator not present
        //    use DEFAULT (1.f ?)
        // compute ratio
        // cents = cents_per_octave * ratio
        // if error
        //   record error
        //   return 0
        // return value
    }


    std::string trim(std::string const& str) {
        if(str.empty())
            return str;

        std::size_t firstScan = str.find_first_not_of(' ');
        std::size_t first     = firstScan == std::string::npos ? str.length() : firstScan;
        std::size_t last      = str.find_last_not_of(' ');
        return str.substr(first, last-first+1);
    }

    bool isComment(std::string const& str) {
        return (str.find('!') == 0);
    }

    int convertToInteger(std::string const& str) {
        int intVal = 0;
        try {
            intVal = stoi(str);
        } catch (const std::invalid_argument& ia) {
            reportError("Invalid integer: " + str);
        }
        return intVal;
    }

    float convertToFloat(std::string const& str) {
        float floatVal = 0;
        try {
            floatVal = stof(str);
        } catch (const std::invalid_argument& ia) {
            reportError("Invalid float: " + str);
        }       
        return floatVal;
    }

    void reportError(std::string const& msg) {
        DEBUG("Error: Scala file '%s': %s", filepath.c_str(), msg.c_str());
        errorCount ++;
    }

};




template <int SIZE>
bool ScalaScale<SIZE>::loadFomFile(std::string filepath) {
    ScalaParser<SIZE> parser;
    bool loaded = parser.importFromFile(filepath); 
    if (loaded) {
        clear();
        description = parser.tempScale.description;
        numPitches = parser.tempScale.numPitches;
        memcpy(cents, parser.tempScale.cents, sizeof(cents)); 
    }
    return loaded;
}

#endif // _tc_scala_scale_hpp_