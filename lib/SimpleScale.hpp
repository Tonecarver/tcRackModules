#ifndef _tc_simple_scale_hpp_
#define _tc_simple_scale_hpp_

#include <iostream>
#include <fstream>
#include <string>

const int majorScaleIntervals[] = { 0, 2, 4, 5, 7, 9, 11 };

// Raw relative pitch information 
// The pitch with value 0 marks the cycle start point
template<int NUM_PITCHES>
struct ScaleTemplate {
    std::string name;
    int numPitches;
    int pitches[ NUM_PITCHES ];
    int cycleStartIndex;


    ScaleTemplate() {
        name = "";        
        memset(pitches, 0, sizeof(pitches));
        cycleStartIndex = 0;
        numPitches = std::min(NUM_PITCHES, 7); // major
        for (int k = 0; k < numPitches; k++) {
            pitches[k] = majorScaleIntervals[k];
        }
    }

    void clear() {
        name.clear();
        numPitches = 0;
        memset(pitches, 0, sizeof(pitches));
        cycleStartIndex = 0;
    }

    void addPitch(int pitch) {
//        if (pitch == 0) {
//            cycleStartIndex = numPitches;
//        }
        pitches[numPitches] = pitch;
        numPitches++;
    }

    bool loadFromFile(const std::string & filepath);
};

enum ScaleExtendDirection { 
    SCALE_EXTEND_UPWARD,
    SCALE_EXTEND_REFLECT    // 
};


template<int NUM_PITCHES>
struct SimpleScale {
    const ScaleTemplate<NUM_PITCHES> * pScaleTemplate; 
    int pitches[ NUM_PITCHES ];
    int rootPitch;
    ScaleExtendDirection  extendDirection;

    SimpleScale() {
        rootPitch = 0;
        extendDirection = SCALE_EXTEND_UPWARD;
        pScaleTemplate = NULL;
    }

    void setScaleTemplate(ScaleTemplate<NUM_PITCHES> const & scaleTemplate) {
        pScaleTemplate = &scaleTemplate;
    }

    void clear() {
        memset(pitches, 0, sizeof(pitches));
    }

    inline int size() const { 
        return NUM_PITCHES;
    }

    void setRootPitch(int rootPitch) {
        this->rootPitch = rootPitch;
        assignScaleNotes();
    }

    void setExtendDirection(ScaleExtendDirection  extendDirection) {
        this->extendDirection = extendDirection;
        assignScaleNotes();
    }

protected:
    void assignScaleNotes() {
        memset(pitches, 0, sizeof(pitches));

        if (! pScaleTemplate) {
            return;
        }

        if (extendDirection == SCALE_EXTEND_UPWARD) {
            assignNotesIncreasing();
        }        
        else if (extendDirection == SCALE_EXTEND_REFLECT) {
            assignNotesReflecting();
        }        
    }

    void assignNotesIncreasing() {

        int delta = 0 - pScaleTemplate->pitches[0];

        int scale_idx = 0;
        for (int k = 0; k < NUM_PITCHES; k++) {
            int pitch = pScaleTemplate->pitches[scale_idx];

            pitches[k] = clamp(pitch + rootPitch + delta, 0, 120); // 120 = max pitch for VCV at 1 volt/octave

            scale_idx += 1;
            if (scale_idx >= pScaleTemplate->numPitches) {
                scale_idx = pScaleTemplate->cycleStartIndex;
                int next_octave = 12 * ((pitch / 12) + 1);
                delta += next_octave;
            }
        }
    }

    void assignNotesReflecting() {
        int delta = 0 - pScaleTemplate->pitches[0];

        bool increasing = true;

        int scale_idx = 0;
        for (int k = 0; k < NUM_PITCHES; k++) {
            int pitch = pScaleTemplate->pitches[scale_idx];

            pitches[k] = clamp(pitch + rootPitch + delta, 0, 120); // 120 = max itch for VCV at 1 volt/octave

            if (increasing) {
                scale_idx ++;
                if (scale_idx >= pScaleTemplate->numPitches) {
                    scale_idx = clamp(scale_idx - 2, 0, pScaleTemplate->numPitches - 1);
                    increasing = false;
                }
            } else {
                scale_idx --;
                if (pitch == 0 || scale_idx < 0) {
                    scale_idx = clamp(scale_idx + 2, 0, pScaleTemplate->numPitches - 1);
                    increasing = true;
                }
            }
        }
    }
};



template<int NUM_PITCHES>
struct ScaleTemplateParser {
    ScaleTemplate<NUM_PITCHES> scaleUnderConstruction;
    int cycleLength;
    int cycleIndex; 
    std::string filepath; 
    int errorCount; 

    bool haveName = false; 

    ScaleTemplateParser() {
        cycleLength = 0;
        cycleIndex = 0; 
    }

    bool importFromFile(const std::string & filepath) {

        this->filepath = filepath;

        errorCount = 0;
        scaleUnderConstruction.clear();

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

        return errorCount == 0;
    }

    void processLine(const std::string& line) {

        //DEBUG("  simple scale parser: line = '%s'", line.c_str());

        std::string str = removeComments(line);
        str = trim(str);
        if (str.length() <= 0) {
            return; // ignore blank lines 
        }

        if (! haveName) {
            scaleUnderConstruction.name = str;
            haveName = true;
        } else {
            int pitch = convertToInteger(str);
            if (scaleUnderConstruction.numPitches >= NUM_PITCHES) {
                reportError("Maximum number of pitches reached: ignoring further input");
            } else {
                scaleUnderConstruction.addPitch(pitch);
                DEBUG(" -- Add pitch %d", pitch);
            }
        }
    }

// TODO: move to general string maniplation class or namespace 
    std::string trim(std::string const& str) {
        if(str.empty())
            return str;

        std::size_t firstScan = str.find_first_not_of(' ');
        std::size_t first     = firstScan == std::string::npos ? str.length() : firstScan;
        std::size_t last      = str.find_last_not_of(' ');
        return str.substr(first, last-first+1);
    }

    std::string removeComments(std::string const& str) {
        if(str.empty())
            return str;

        std::size_t pos = str.find(';');
        if (pos == std::string::npos) {
            return str;
        }

        return str.substr(0, pos);
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
        DEBUG("Error: Simple Scale file '%s': %s", filepath.c_str(), msg.c_str());
        errorCount ++;
    }

};


template<int NUM_PITCHES>
bool ScaleTemplate<NUM_PITCHES>::loadFromFile(const std::string & filepath) {
    ScaleTemplateParser<NUM_PITCHES> parser;
    bool loaded = parser.importFromFile(filepath); 
    if (loaded) {
        clear();
        name = parser.scaleUnderConstruction.name;
        numPitches = parser.scaleUnderConstruction.numPitches;
        memcpy(pitches, parser.scaleUnderConstruction.pitches, sizeof(parser.scaleUnderConstruction.pitches)); 
    }
    return loaded;
}

#endif // _tc_simple_scale_hpp_
