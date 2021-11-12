# scalemaker.py
import sys
sys.dont_write_bytecode = True

import argparse
import re

COMMENT_MARKER = ';'

enforceCycleStart = False 


noteRegex = '^([ABCDEFGabcdefg][#b]?)([0-9]*)$'
noteMatcher = re.compile(noteRegex)

noteOffsets = {
    "C" : 0,
    "D" : 2,
    "E" : 4,
    "F" : 5,
    "G" : 7,
    "A" : 9,
    "B" : 11,

    "Cb" : 11,
    "Db" : 1,
    "Eb" : 3,
    "Fb" : 4,
    "Gb" : 6,
    "Ab" : 8,
    "Bb" : 10,

    "C#" : 1,
    "D#" : 3,
    "E#" : 5,
    "F#" : 6,
    "G#" : 8,
    "A#" : 10,
    "B#" : 0,
}

errorCount = 0
def reportError(msg):
    global errorCount   
    print(f'ERROR: {msg}')
    errorCount += 1

class Scale:
    def __init__(self, name):
        self.name = name
        self.pitches = [] #  relative pitch values         
                          #   0 == first note in repeat cycle
                          # < 0 == notes preceding the repeat cycle (like handpan ding, for example)
                          # > 0 = delta from first note in repeat cycle
        self.cycle_start_idx = 0
        self.cycle_length = 1

    def addPitch(self, pitch):
        self.pitches.append(pitch)
        self.pitches.sort()

    def addCycleStart(self, pitch):
        self.cycle_start_idx = len(self.pitches)
        self.pitches.append(pitch)
        self.pitches.sort()

    def getCycleStartIdx(self):
        return self.cycle_start_idx

    def normalize(self):
        cycleStartPitch = self.pitches[self.cycle_start_idx]
        self.cycle_length = 0
        for k in range(0, len(self.pitches)):
            self.pitches[k] -= cycleStartPitch
            if self.pitches[k] >= 0:
                self.cycle_length += 1

    def size(self):
        return len(self.pitches)

#    def getIntervals(self):
#        if not self.intervals:
#            self._computeIntervals()
#        return self.intervals
#        # TODO: lazy create intervals, populate up to 16 ?? 
#        # for handpan scales, the repeat cycle does not always start on the lowest note
#        # do smart repetitiion - find the repeat point following the highest pitch
#        # if specifed s a lower pitch, use the lower pitch as the cycle point
#        # if not specified, use pitch[0]
#
#    def _computeIntervals(self):
#        self.intervals = []
#        if self.pitches:
#            prevPitch = None
#            for pitch in self.pitches:
#                if prevPitch: 
#                    delta = pitch - prevPitch
#                    self.intervals.append(delta)
#                prevPitch = pitch
#            
#            basePitch = self.pitches[0]
#            while basePitch < prevPitch:
#                basePitch += 12
#            delta = basePitch - prevPitch
#            self.intervals.append(delta)

    def __str__(self):
        return '{}: pitches={}, cycleStart={}, cycle_length={}'.format(self.name, self.pitches, self.cycle_start_idx, self.cycle_length)
    def __repr__(self):
        return self.__str__()

def getPitchForName(name):
    note = None
    matchObject = noteMatcher.search(name)
    if matchObject:
        note = matchObject.group(1)
        octave = matchObject.group(2)

    pitch = noteOffsets[note]
    if octave:
        pitch += int(octave) * 12
    
    #print(f'NAME: {name}, note = {note}, octave = {octave}, pitch = {pitch}')
    if not note:
        reportError(f'Unable to determine pitch for {name}')
        pitch = 0

    if pitch > 120: 
        reportError(f'Pitch {pitch} for {name} is > 120')

    return pitch

def createScale(line):
    parts = line.split('=')
    if not parts:
        reportError(f'Badly formed line: {line}')
        return
    name = parts[0].strip()
    print(f'-- scale  {name}')
    scale = Scale(name)
    octaves = 0
    prevPitch = 0
    pitchNames = parts[1].split()
    for pitchName in pitchNames:
        cycleStart = False
        if pitchName:
            if pitchName.startswith('/'):
                pitchName = pitchName[1:]
                if enforceCycleStart: 
                    cycleStart = True

            pitch = getPitchForName(pitchName)
            if pitch < prevPitch:
                #print(f'  pitch {pitch} < prevPitch {prevPitch}, increase octave')
                octaves += 12

            #print(f' add pitch {pitch + octaves}')
            if cycleStart:
                scale.addCycleStart(pitch + octaves)
            else:
                scale.addPitch(pitch + octaves)
            prevPitch = pitch

    scale.normalize()
    return scale

def extendScale(scale, rootPitch, extendSize):

    delta = 0 - scale.pitches[0]
    rooted_pitches = []

    print(f'------- Extend Scale: {scale.name}, delta is {delta}')
    prevPitch = 0
    octaves = 0
    cycle_start_idx = scale.getCycleStartIdx()
    scale_idx = 0
    for k in range(0, extendSize):
        pitch = scale.pitches[scale_idx]

        rooted_pitches.append(pitch + rootPitch + delta)

        scale_idx += 1
        if scale_idx >= len(scale.pitches):
            scale_idx = cycle_start_idx
            next_octave = 12 * int((pitch / 12) + 1)
            delta += next_octave

    print(f'Extend {scale.name}, rootPitch {rootPitch}, rooted_pitches = {rooted_pitches} ')

def loadScaleDefinitions(filename):
    scales = [] 

    with open(filename, 'r') as f:
        while True:
            line = f.readline()  
            if not line:
                break     

            # strip comments
            pos = line.find(COMMENT_MARKER)
            line = line[:pos]
            line = line.strip()

            if line:
                scale = createScale(line)
                if scale: 
                    scales.append(scale)
                    print(f'Scale: {scale}')
                    #print('  intervals {}'.format(scale.getIntervals()))
    return scales

def isIntervalMatch(scale_A, scale_B):
    if len(scale_A.pitches) != len(scale_B.pitches):
        return False

    for idx in range(0, len(scale_A.pitches)):
        if scale_A.pitches[idx] != scale_B.pitches[idx]:
            return False
    
    return True

def reportDuplicateScales(scales):
    for scale in scales:
        for other_scale in scales:
            if scale != other_scale:
                if scale.name == other_scale.name:
                    print('ERROR: scale named "{}" is multiply defined'.format(scale.name))
                if isIntervalMatch(scale, other_scale):
                    print('WARNING: duplicate scale intervals: "{}" and "{}"'.format(scale.name,other_scale.name))

def writeScaleFile(scale, path):
    with open(path, 'wt') as f:
        f.write('; scale interval file: scale "{}" has {} pitch intervals\n'.format(scale.name, len(scale.pitches)))
        f.write('{}\n'.format(scale.name))
        for pitch in scale.pitches:
            f.write('{}\n'.format(pitch))


if __name__ == "__main__":

    parser = argparse.ArgumentParser(description='Scale maker')
    parser.add_argument('--scalefile', required=True, help='the file containing the scale defintions')
    parser.add_argument('--outfolder', required=True, help='the folder to write the scale files to')

    args = parser.parse_args()

    scalefile = args.scalefile
    outfolder = args.outfolder

    scales = [] 
    scales = loadScaleDefinitions(scalefile)

    reportDuplicateScales(scales)

    #for scale in scales:
    #    extendScale(scale, 0, 16) 
    # print(scales)

    import os
    print(os.getcwd())

    for scale in scales:
        path = '{}/{}.ivl'.format(outfolder, scale.name)
        writeScaleFile(scale, path) 
