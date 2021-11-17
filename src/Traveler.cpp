#include "plugin.hpp"
#include <mutex>
#include <widget/FramebufferWidget.hpp>
#include <ui/Label.hpp>
#include <osdialog.h>
#include "../lib/FastRandom.hpp"
#include "../lib/FloatCounter.hpp"
#include "../lib/List.hpp"
#include "../lib/ThreadSafeList.hpp"
#include "../lib/PtrArray.hpp"
#include "../lib/SampleRateCalculator.hpp"
// #include "TravelerExpander.hpp"


#include "../lib/SimpleScale.hpp"


const int ROVER_FIELD_DIMENSION = 16;
const int ROVER_FIELD_ROWS = ROVER_FIELD_DIMENSION;
const int ROVER_FIELD_COLUMNS = ROVER_FIELD_DIMENSION;
const float CELL_SIZE_MM = 6.0;
const float EDGE_LENGTH_MM = 94.627;
const int NUM_WALLS = 4; 

const int MAX_POLYPHONY = 16;


const float MIN_BPM = 15.f;
const float MAX_BPM = 960.f; 
const float DFLT_BPM = 120.f;

const float MAX_NOTE_LENGTH_BEATS = 2.0;
const float MIN_NOTE_LENGTH_BEATS = 1.f / 128.f;

const int DFLT_ROOT_NOTE = 36; // C3 
const int NOTE_OFFSET_RANGE = 36; // 3 octaves: walls can vary octave +/- this much from global rootNote

const int COLLISION_LIMIT = 10;
const int STUCK_LIMIT = 10;

const std::string FILE_NOT_SELECTED_STRING = "<empty>";

struct MatrixDimensions { 
    int num_rows; 
    int num_cols;

    MatrixDimensions(int rows, int cols) {
        num_rows = rows;
        num_cols = cols;
    }

    void setRowsCols(int rows, int cols) {
        num_rows = rows;
        num_cols = cols;
    }

    inline bool isNorthRow(int row) {
        return row == 0;
    }

    inline bool isEastColumn(int col) {
        return col == (num_cols -1);
    }

    inline bool isSouthRow(int row) {
        return row == (num_rows - 1);
    }

    inline bool isWestColumn(int col) {
        return col == 0;
    }

    inline int getNorthRow() { 
        return 0;
    }

    inline int getEastColumn() { 
        return num_cols - 1;
    }

    inline int getSouthRow() { 
        return num_rows - 1;
    }

    inline int getWestColumn() { 
        return 0;
    }

    inline bool canMoveNorth(int row) {
        return row > 0;
    }

    inline bool canMoveEast(int col) {
        return col < (num_cols - 1);       
    }

    inline bool canMoveSouth(int row) {
        return row < (num_rows - 1);
    }

    inline bool canMoveWest(int col) {
        return col > 0;        
    }

	inline bool isInside(int row, int col) {
		return (0 <= row && row < num_rows) && (0 <= col && col < num_cols);
	}

	inline bool isOutside(int row, int col) {
		return (! isInside(row, col));
	}

};


enum RoverType { 
    ROVER_NORTH,
    ROVER_NORTHEAST,
    ROVER_EAST,
    ROVER_SOUTHEAST,
    ROVER_SOUTH,
    ROVER_SOUTHWEST,
    ROVER_WEST,
    ROVER_NORTHWEST,
    NUM_ROVER_TYPES
};

// Direction heading flags (bitmask)
const int HEADING_NORTH = 1;
const int HEADING_EAST  = 2;
const int HEADING_SOUTH = 4;
const int HEADING_WEST  = 8; 
const int HEADING_NORTHEAST = (HEADING_NORTH | HEADING_EAST);
const int HEADING_SOUTHEAST = (HEADING_SOUTH | HEADING_EAST);
const int HEADING_SOUTHWEST = (HEADING_SOUTH | HEADING_WEST);
const int HEADING_NORTHWEST = (HEADING_NORTH | HEADING_WEST);

struct RoverRedirect { 
	int directionFlags;
    RoverType reverse;
    RoverType rotate90Cw;
    RoverType rotate90Ccw;
    RoverType rotate45Cw;
    RoverType rotate45Ccw;
}; 

const RoverRedirect roverSteeringTable[NUM_ROVER_TYPES] = {
                   //   directionFlags,   reverse,         rotate90Cw,      rotate90Ccw,     rotate45Cw,      rotate45Ccw
/* ROVER_NORTH     */ { HEADING_NORTH,     ROVER_SOUTH,     ROVER_EAST,      ROVER_WEST,      ROVER_NORTHEAST, ROVER_NORTHWEST },
/* ROVER_NORTHEAST */ { HEADING_NORTHEAST, ROVER_SOUTHWEST, ROVER_SOUTHEAST, ROVER_NORTHWEST, ROVER_EAST,      ROVER_NORTH     },
/* ROVER_EAST      */ { HEADING_EAST,      ROVER_WEST,      ROVER_SOUTH,     ROVER_NORTH,     ROVER_SOUTHEAST, ROVER_NORTHEAST },
/* ROVER_SOUTHEAST */ { HEADING_SOUTHEAST, ROVER_NORTHWEST, ROVER_SOUTHWEST, ROVER_NORTHEAST, ROVER_SOUTH,     ROVER_EAST      },
/* ROVER_SOUTH     */ { HEADING_SOUTH,     ROVER_NORTH,     ROVER_WEST,      ROVER_EAST,      ROVER_SOUTHWEST, ROVER_SOUTHEAST },
/* ROVER_SOUTHWEST */ { HEADING_SOUTHWEST, ROVER_NORTHEAST, ROVER_NORTHWEST, ROVER_SOUTHEAST, ROVER_WEST,      ROVER_SOUTH     },
/* ROVER_WEST      */ { HEADING_WEST,      ROVER_EAST,      ROVER_NORTH,     ROVER_SOUTH,     ROVER_NORTHWEST, ROVER_SOUTHWEST },
/* ROVER_NORTHWEST */ { HEADING_NORTHWEST, ROVER_SOUTHEAST, ROVER_NORTHEAST, ROVER_SOUTHWEST, ROVER_NORTH,     ROVER_WEST      }
};

struct RoverReflect { 
    RoverType reflectNorth; // reflect off north wall 
    RoverType reflectEast;  // reflect off east wall 
    RoverType reflectSouth; // reflect off south wall 
    RoverType reflectWest;  // reflect off west wall 
}; 

const RoverReflect roverReflectionTable[NUM_ROVER_TYPES] = {
                   //   reflectNorth,    reflectEast,     reflectSouth,    reflectWest 
/* ROVER_NORTH     */ { ROVER_SOUTH,     ROVER_NORTH,     ROVER_NORTH,     ROVER_NORTH     },
/* ROVER_NORTHEAST */ { ROVER_SOUTHEAST, ROVER_NORTHWEST, ROVER_NORTHEAST, ROVER_NORTHEAST },
/* ROVER_EAST      */ { ROVER_EAST,      ROVER_WEST,      ROVER_EAST,      ROVER_EAST      },
/* ROVER_SOUTHEAST */ { ROVER_SOUTHEAST, ROVER_SOUTHWEST, ROVER_NORTHEAST, ROVER_SOUTHEAST },
/* ROVER_SOUTH     */ { ROVER_SOUTH,     ROVER_SOUTH,     ROVER_NORTH,     ROVER_SOUTH     },
/* ROVER_SOUTHWEST */ { ROVER_SOUTHWEST, ROVER_SOUTHWEST, ROVER_NORTHWEST, ROVER_SOUTHEAST },
/* ROVER_WEST      */ { ROVER_WEST,      ROVER_WEST,      ROVER_WEST,      ROVER_EAST      },
/* ROVER_NORTHWEST */ { ROVER_SOUTHWEST, ROVER_NORTHWEST, ROVER_NORTHWEST, ROVER_NORTHEAST },
};


enum ObstacleType { 
    OBSTACLE_MIRROR_HORIZONTAL,
	OBSTACLE_MIRROR_HORIZONTAL_FLIP,
    OBSTACLE_MIRROR_VERTICAL,
    OBSTACLE_MIRROR_VERTICAL_FLIP, 
    OBSTACLE_MIRROR_SQUARE,
    OBSTACLE_MIRROR_DIAMOND,
    OBSTACLE_MIRROR_NE_SW,
    OBSTACLE_MIRROR_NE_SW_FLIP,
    OBSTACLE_MIRROR_NW_SE,
    OBSTACLE_MIRROR_NW_SE_FLIP,
	OBSTACLE_WEDGE_NORTH,
	OBSTACLE_WEDGE_EAST,
	OBSTACLE_WEDGE_SOUTH,
	OBSTACLE_WEDGE_WEST,
	OBSTACLE_DETOUR,
	OBSTACLE_SPIN,
	OBSTACLE_WORMHOLE,
	OBSTACLE_SPEED_NORMAL,
	OBSTACLE_SPEED_TOGGLE,
	OBSTACLE_SPEED_ALT,

	// TODO: add Pause - pause other rovers 
	// add Lag (tar pit), pause this rover 

    NUM_OBSTACLE_TYPES
};


enum SpeedType { 
    SPEED_NORMAL,
    SPEED_ALT,
    NUM_SPEED_TYPES
};

struct Occupant : ListNode {
    int pos_row;
    int pos_col;
    bool isRover;
    MatrixDimensions * pMatrixDimensions;
    int birthday; // for age calculations 

    Occupant() {
        pMatrixDimensions = NULL;
        isRover = false;
        setPosition(0, 0);
        birthday = 0;
    }

    void initialize(int row, int col, int birthday, bool isRover, MatrixDimensions & matrixDimensions) {
        this->pMatrixDimensions = &matrixDimensions;
        this->isRover = isRover;
		this->birthday = birthday;
        setPosition(row, col);
    }

    void setPosition(int row, int col) {
        pos_row = row;
        pos_col = col;
    }

    int getCellId() const { 
        return (pos_row << 4) | pos_col;  // << assumes MAX SIZE is 16
    }

    int getRow() const { 
        return pos_row; 
    }

    int getCol() const { 
        return pos_col; 
    }

    int getBirthday() const {
        return birthday;
    }

	void clamp() { 
		pos_row = ::clamp(pos_row, 0, ROVER_FIELD_ROWS - 1);
		pos_col = ::clamp(pos_col, 0, ROVER_FIELD_COLUMNS - 1) ;
	}

    void moveNorth() {
        if (pMatrixDimensions->canMoveNorth(pos_row)) {
            pos_row--;
        }
    }

    void moveNorthEast() {
        if (pMatrixDimensions->canMoveNorth(pos_row) && pMatrixDimensions->canMoveEast(pos_col)) {
            pos_row--;
            pos_col++;
        }
    }

    void moveEast() {
        if (pMatrixDimensions->canMoveEast(pos_col)) {
            pos_col++;
        }
    }

    void moveSouthEast() {
        if (pMatrixDimensions->canMoveSouth(pos_row) && pMatrixDimensions->canMoveEast(pos_col)) {
            pos_row++;
            pos_col++;
        }
    }

    void moveSouth() {
        if (pMatrixDimensions->canMoveSouth(pos_row)) {
            pos_row++;
        }
    }

    void moveSouthWest() {
        if (pMatrixDimensions->canMoveSouth(pos_row) && pMatrixDimensions->canMoveWest(pos_col)) {
            pos_row++;
            pos_col--;
        }
    }

    void moveWest() {
        if (pMatrixDimensions->canMoveWest(pos_col)) {
            pos_col--;
        }
    }

    void moveNorthWest() {
        if (pMatrixDimensions->canMoveNorth(pos_row) && pMatrixDimensions->canMoveWest(pos_col)) {
            pos_row--;
            pos_col--;
        }
    }

    void moveToNorthRow() {
        pos_row = 0;
    }

    void moveToEastColumn() {
        pos_col = pMatrixDimensions->num_cols - 1;
    }

    void moveToSouthRow() {
        pos_row = pMatrixDimensions->num_rows - 1;
    }

    void moveToWestColumn() {
        pos_col = 0;
    }

	void moveToNorthEastCorner() { 
        pos_row = 0;
        pos_col = pMatrixDimensions->num_cols - 1;
	}

	void moveToSouthEastCorner() { 
        pos_row = pMatrixDimensions->num_rows - 1;
        pos_col = pMatrixDimensions->num_cols - 1;
	}
	
	void moveToSouthWestCorner() { 
        pos_row = pMatrixDimensions->num_rows - 1;
        pos_col = 0;
	}
	
	void moveToNorthWestCorner() { 
        pos_row = 0;
        pos_col = 0;
	}

	void moveHorizontal(int amount) {
		int num_cols = pMatrixDimensions->num_cols;
		pos_col += amount;
		while (pos_col < 0) {
			pos_col += num_cols; // wrap 
		}
		while (pos_col >= num_cols) {
			pos_col -= num_cols;
		}
	}	

	void moveVertical(int amount) {
		int num_rows = pMatrixDimensions->num_rows;
		pos_row += amount;
		while (pos_row < 0) {
			pos_row += num_rows; // wrap 
		}
		while (pos_row >= num_rows) {
			pos_row -= num_rows; // wrap
		}
	}	

    bool canMoveNorth() const {
        return pMatrixDimensions->canMoveNorth(pos_row);
    }

    bool canMoveEast() const {
        return pMatrixDimensions->canMoveEast(pos_col);
    }

    bool canMoveSouth() const {
        return pMatrixDimensions->canMoveSouth(pos_row);
    }

    bool canMoveWest() const {
        return pMatrixDimensions->canMoveWest(pos_col);
    }

	bool isAtWall() const {
        return isAtNorthWall() || isAtEastWall() || isAtSouthWall() || isAtWestWall();
	}

	bool isAtNorthEastCorner() const {
        return isAtNorthWall() && isAtEastWall();
	}

	bool isAtSouthEastCorner() const {
        return isAtSouthWall() && isAtEastWall();
	}

	bool isAtSouthWestCorner() const {
        return isAtSouthWall() && isAtWestWall();
	}

	bool isAtNorthWestCorner() const {
        return isAtNorthWall() && isAtWestWall();
	}

    bool isAtNorthWall() const {
        return pMatrixDimensions->isNorthRow(pos_row);
    }    

    bool isAtEastWall() const {
        return pMatrixDimensions->isEastColumn(pos_col);
    }    

    bool isAtSouthWall() const {
        return pMatrixDimensions->isSouthRow(pos_row);
    }    

    bool isAtWestWall() const {
        return pMatrixDimensions->isWestColumn(pos_col);
    }    

	inline bool isInsideWalls() const {
        return pMatrixDimensions->isInside(pos_row, pos_col);
	}

	inline bool isOutsideWalls() const {
        return (! pMatrixDimensions->isInside(pos_row, pos_col));
	}
};



struct Rover : Occupant { 
    RoverType roverType; 
    SpeedType speed;
	int collisionCount; // 0 for not in a collision, >0 number of serial collision attempts
	int stuckCount;     // 0 for not stuck, >0 number of serial inability to move (move and return to same place)

    Rover() {
        roverType = ROVER_NORTH;
        speed = SPEED_NORMAL;
		collisionCount = 0;
		stuckCount = 0;
    }

    void initialize(RoverType roverType, int row, int col, int birthday, MatrixDimensions & matrixDimensions) {
        Occupant::initialize(row, col, birthday, true, matrixDimensions);
        this->roverType = roverType;
        this->speed = SPEED_NORMAL;
		this->collisionCount = 0;
    }

    RoverType getRoverType() const { 
        return roverType;
    }

	void setType(RoverType roverType) {
		this->roverType = roverType;
	}

    SpeedType getSpeed() const {
        return this->speed;
    }

    void setSpeed(SpeedType speed) {
        this->speed = speed;
    }

	void toggleSpeed() { 
		speed = (speed == SPEED_NORMAL ? SPEED_ALT : SPEED_NORMAL);
	}

    int getCollisionCount() const {
        return this->collisionCount;
    }

    bool isColliding() const {
        return this->collisionCount > 0;
    }

    void setColliding(bool colliding) {
		collisionCount = (colliding ? collisionCount + 1 : 0);
    }

	int getStuckCount() const { 
		return stuckCount;
	}

	bool isStuck() const { 
		return stuckCount > 0;
	}

	void setStuck(bool stuck) { 
		stuckCount = (stuck ? stuckCount + 1 : 0);
	}
	
    void advance() {
        switch (roverType) {
            case ROVER_NORTH:
                moveNorth();
                break;

            case ROVER_NORTHEAST:
                moveNorthEast();
                break;

            case ROVER_EAST:
                moveEast();
                break;

            case ROVER_SOUTHEAST:
                moveSouthEast();
                break;

            case ROVER_SOUTH:
                moveSouth();
                break;

            case ROVER_SOUTHWEST:
                moveSouthWest();
                break;

            case ROVER_WEST:
                moveWest();
                break;

            case ROVER_NORTHWEST:
                moveNorthWest();
                break;
			default:
				break;
        }	
    }

    inline void reverse() {
        roverType = roverSteeringTable[ int(roverType) ].reverse;
    }

	inline void reflectNorth() {
		roverType = roverReflectionTable[roverType].reflectNorth;
	}

	inline void reflectEast() {
		roverType = roverReflectionTable[roverType].reflectEast;
	}

	inline void reflectSouth() {
		roverType = roverReflectionTable[roverType].reflectSouth;
	}

	inline void reflectWest() {
		roverType = roverReflectionTable[roverType].reflectWest;
	}

    inline void rotate90Cw() {
        roverType = roverSteeringTable[ int(roverType) ].rotate90Cw;
    }

    void rotate90Ccw() {
        roverType = roverSteeringTable[ int(roverType) ].rotate90Ccw;
    }

    void rotate45Cw() {
        roverType = roverSteeringTable[ int(roverType) ].rotate45Cw;
    }

    void rotate45Ccw() {
        roverType = roverSteeringTable[ int(roverType) ].rotate45Ccw;
    }

	void moveLateralCw() {
        switch (roverType) {
            case ROVER_NORTH:
                moveWest();
                break;

            case ROVER_NORTHEAST:
                moveNorth();
                break;

            case ROVER_EAST:
                moveNorth();
                break;

            case ROVER_SOUTHEAST:
                moveEast();
                break;

            case ROVER_SOUTH:
                moveEast();
                break;

            case ROVER_SOUTHWEST:
                moveSouth();
                break;

            case ROVER_WEST:
                moveSouth();
                break;

            case ROVER_NORTHWEST:
                moveWest();
                break;
			default:
				break;
        }	
	}

	void moveLateralCcw() {
        switch (roverType) {
            case ROVER_NORTH:
                moveEast();
                break;

            case ROVER_NORTHEAST:
                moveEast();
                break;

            case ROVER_EAST:
                moveSouth();
                break;

            case ROVER_SOUTHEAST:
                moveSouth();
                break;

            case ROVER_SOUTH:
                moveWest();
                break;

            case ROVER_SOUTHWEST:
                moveWest();
                break;

            case ROVER_WEST:
                moveNorth();
                break;

            case ROVER_NORTHWEST:
                moveNorth();
                break;
			default:
				break;
        }	
	}

    void goNorth() {
        roverType = ROVER_NORTH;
    }

    void goEast() {
        roverType = ROVER_EAST;
    }

    void goSouth() {
        roverType = ROVER_SOUTH;
    }

    void goWest() {
        roverType = ROVER_WEST;
    }

    bool isHorizontal() const {
        return (roverType == ROVER_EAST || roverType == ROVER_WEST);
    }

    bool isVertical() const {
        return (roverType == ROVER_NORTH || roverType == ROVER_SOUTH);
    }

    bool isDiagonal() const {
        return (roverType == ROVER_NORTHEAST || roverType == ROVER_SOUTHEAST || roverType == ROVER_SOUTHWEST || roverType == ROVER_NORTHWEST);
    }

    bool isNorth() const {
        return (roverType == ROVER_NORTH);
    }

    bool isEast() const {
        return (roverType == ROVER_EAST);
    }

    bool isSouth() const {
        return (roverType == ROVER_SOUTH);
    }

    bool isWest() const {
        return (roverType == ROVER_WEST);
    }

    bool isNorthEast() const {
        return (roverType == ROVER_NORTHEAST);
    }

    bool isNorthWest() const {
        return (roverType == ROVER_NORTHWEST);
    }

     bool isSouthEast() const {
        return (roverType == ROVER_SOUTHEAST);
    }

    bool isSouthWest() const {
        return (roverType == ROVER_SOUTHWEST);
    }

	bool isNorthbound() const {
		return (roverSteeringTable[ int(roverType) ].directionFlags & HEADING_NORTH);
	}

	bool isEastbound() const { 
		return (roverSteeringTable[ int(roverType) ].directionFlags & HEADING_EAST);
	}

	bool isSouthbound() const { 
		return (roverSteeringTable[ int(roverType) ].directionFlags & HEADING_SOUTH);
	}

	bool isWestbound() const { 
		return (roverSteeringTable[ int(roverType) ].directionFlags & HEADING_WEST);
	}

	inline bool isAtNorthCorner() const{ 
		return isAtNorthWall() && ((isNorthEast() && isAtEastWall()) || (isNorthWest() && isAtWestWall()));
	}

	inline bool isAtEastCorner() const { 
		return isAtEastWall() && ((isNorthEast() && isAtNorthWall()) || (isSouthEast() && isAtSouthWall()));
	}

	inline bool isAtSouthCorner() const { 
		return isAtSouthWall() && ((isSouthEast() && isAtSouthWall()) || (isSouthWest() && isAtWestWall()));
	}

	inline bool isAtWestCorner() const { 
		return isAtWestWall() && ((isNorthWest() && isAtNorthWall()) || (isSouthWest() && isAtSouthWall()));
	}


};

struct Obstacle : Occupant { 
    ObstacleType obstacleType; 
	int hitCount;

    Obstacle() {
        this->obstacleType = OBSTACLE_MIRROR_HORIZONTAL;
		hitCount = 0;
    }

    void initialize(ObstacleType obstacleType, int row, int col, int birthday, MatrixDimensions & matrixDimensions) {
        Occupant::initialize(row, col, birthday, false, matrixDimensions);
        this->obstacleType = obstacleType;
		hitCount = 0;
    }

	// Return 0 or 1 to indicate alternating selection
	int selectAlternating() { 
		hitCount = (hitCount + 1) & 1;
		return hitCount;
	}
};

const int MAX_PITCH = 120;  // 10 octaves 
const int MIN_PITCH = 0; 
const int NUM_OCTAVES = 10; 

const std::string noteNames[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
const std::string octaveNames[11] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };



// 	inline int getOctaveForPitch(int pitch) {
// 		return pitch / 12;
// 	}

// 	inline int getNoteOffsetForPitch(int pitch) {
// 		return pitch % 12;
// 	}



enum WallDirection { 
	WALL_NORTH,
	WALL_EAST,
	WALL_SOUTH,
	WALL_WEST
}; 


struct Wall { 
	WallDirection direction;
	int rootNote; // 0..120
	int rootNoteOffset; // +/- 3 octaves 
	
	SimpleScale<ROVER_FIELD_DIMENSION> scale;
	bool muted;         // true = suppress output 

	int climb;          // move rovers this much along the wall following a strike

    int rotationAmount;        // translate absolute index to shifted index
	int activeRotationAmount;
	
	int autoRotateAmount;      // rotate this much following one or more strikes
	int activeAutoRotateAmount;
	bool autoRotationPending;

	bool ascending;    // true = pitches in ascending order, false = descending

    // int pitches[ ROVER_FIELD_DIMENSION ];
	int pitchActiveCounters[ ROVER_FIELD_DIMENSION ];
	int pitchIndexTable[ ROVER_FIELD_DIMENSION ]; // remap indexes to implement ascending/descending 

	int numVisibleBlocks;
	int strike_count; 

    Wall(WallDirection direction) {
		this->direction = direction;
		rootNote = DFLT_ROOT_NOTE;
		rootNoteOffset = 0; // C 
		scale.setRootPitch(rootNote + rootNoteOffset);
		// scale.setExtendDirection(SCALE_EXTEND_UPWARD);
		ascending = true; // store pitches in ascending order 

		muted = false; 

		rotationAmount = 0;
		activeRotationAmount = 0;

		autoRotateAmount = 0;
		activeAutoRotateAmount = 0;
		autoRotationPending = false; 

		climb = 0;
		 
		memset(pitchActiveCounters, 0, sizeof(pitchActiveCounters));

        numVisibleBlocks = ROVER_FIELD_DIMENSION;

        for (int k = 0; k < ROVER_FIELD_DIMENSION; k++) {
            //pitches[k] = 36; // C
			pitchIndexTable[k] = k;
        }
		strike_count = 0;
    }

	void clear() { 
		memset(pitchActiveCounters, 0, sizeof(pitchActiveCounters));
		strike_count = 0;
	}

	int getRootNote() const {
		return rootNote;
	}

	int getRootNoteOffset() const {
		return rootNoteOffset;
	}

	int getRotation() const {
		return rotationAmount;
	}

	int getAutoRotate() const {
		return autoRotateAmount;
	}
	
	int getClimb() const {
		return climb;
	}
	
	bool isAscending() const {
		return ascending;
	}

	bool isMuted() const {
		return muted;
	}

	void setRootNote(int rootNote) {
		this->rootNote = rootNote;
		computePitches();
	}

	void setRootNoteOffset(int rootNoteOffset) {
		this->rootNoteOffset = rootNoteOffset;
		computePitches();
	}
	
	void setScaleExtendDirection(ScaleExtendDirection extendDirection) {
		scale.setExtendDirection(extendDirection);
		computePitches();			
	}
	
	void setScaleTemplate(const ScaleTemplate<ROVER_FIELD_DIMENSION> & scaleTemplate) {
		// DEBUG(" WALL: %d, set scale %s", direction, scale.name.c_str());
		scale.setScaleTemplate(scaleTemplate);
		computePitches();			
	}

	void setAscending(bool beAscending) {
		if (beAscending != ascending) {
			ascending = beAscending;
			invertScale();
		}
	}

	void setMuted(bool beMuted) {
		muted = beMuted;
	}

	void setRotation(int amount) {
		rotationAmount = amount;
		activeRotationAmount = clamp(amount, -(ROVER_FIELD_DIMENSION-1), ROVER_FIELD_DIMENSION - 1);
		if (activeRotationAmount < 0) {
			activeRotationAmount += ROVER_FIELD_DIMENSION;
		}
	}

	void setAutoRotate(int amount) {
		autoRotateAmount = amount;
		activeAutoRotateAmount = clamp(amount, -(ROVER_FIELD_DIMENSION-1), ROVER_FIELD_DIMENSION - 1);
		if (activeAutoRotateAmount < 0) {
			activeAutoRotateAmount += ROVER_FIELD_DIMENSION;
		}
	}

	void setClimb(int climb) { 
		this->climb = climb;
	}
	
	void resetRotation() { 
		// reset to a non-autoRotated position 
		activeRotationAmount = rotationAmount;
	}

	int getLowestPitch() const {
		if (ascending) {
			return getPitchAtInterval(0);
		}
		return getPitchAtInterval(ROVER_FIELD_DIMENSION - 1);
	}

	int getHighestPitch() const {
		if (ascending) {
			return getPitchAtInterval(ROVER_FIELD_DIMENSION - 1);
		}
		return getPitchAtInterval(0);
	}

	bool isStrikeActive(int block_idx) const { 
		return pitchActiveCounters[ blockIdxToPitchIdx(block_idx) ] > 0;
	}

    int getPitchForInterval(int interval_idx) const {
        return getPitchAtInterval(interval_idx);
    }

    int getPitchForBlock(int block_idx) const {
        return scale.pitches[ blockIdxToPitchIdx(block_idx) ];
    }

	// return the pitch interval idx resulting from the strike 
	// pass that to endStrike() when strike timer expires 
	int strike(int block_idx) {
		strike_count ++;
		int pitch_idx = blockIdxToPitchIdx(block_idx);
		pitchActiveCounters[pitch_idx]++;
		// DEBUG("Wall[%d] strike, pitch interval %d", direction, pitch_idx);
		autoRotationPending = true;
		return pitch_idx;
	}

	void endStrike(int pitchInterval_idx) {
		//DEBUG("Wall[%d] end strike, pitch interval %d", direction, pitchInterval_idx);
		//int pitch_idx = (pitchInterval_idx + rotationAmount) % ROVER_FIELD_DIMENSION;
		strike_count --;
		pitchActiveCounters[pitchInterval_idx]--;
		if (pitchActiveCounters[pitchInterval_idx] < 0) {
			pitchActiveCounters[pitchInterval_idx] = 0; // safety for underflow 
		}
	}

	void computePitches() {
		int effectiveRootPitch = clamp(rootNote + rootNoteOffset, 0, MAX_PITCH);
		scale.setRootPitch(effectiveRootPitch);
	}

	// Auto Rotation should occur before evaluating for new strikes 
	// but after ending flash/highlight showing previous strike
	void applyAutoRotation() {
		if (autoRotationPending) {
			activeRotationAmount += activeAutoRotateAmount;
			activeRotationAmount %= ROVER_FIELD_DIMENSION;
			autoRotationPending = false;
		}
	}

protected:
	int getPitchAtInterval(int idx) const {
		return scale.pitches[ pitchIndexTable[idx] ];
	}	

	int getPitchCountAtInterval(int idx) const {
		return pitchActiveCounters[ pitchIndexTable[idx] ];
	}	

	int blockIdxToPitchIdx(int block_idx) const {
        return pitchIndexTable[ (block_idx + activeRotationAmount) % ROVER_FIELD_DIMENSION ];
	}

	void invertScale() {
		int temp_pitchIndexTable[ ROVER_FIELD_DIMENSION ]; 		

		memcpy(temp_pitchIndexTable, pitchIndexTable, sizeof(temp_pitchIndexTable));

		for (int k = 0; k < ROVER_FIELD_DIMENSION; k++) {
			pitchIndexTable[ (ROVER_FIELD_DIMENSION - 1) - k] = temp_pitchIndexTable[k];
		}
	}

};

struct Matrix {
    MatrixDimensions dimensions{ROVER_FIELD_ROWS, ROVER_FIELD_COLUMNS};
    Occupant * occupants[ROVER_FIELD_ROWS][ROVER_FIELD_COLUMNS]; 

    Matrix() {
        clearOccupants();
    }

	int getNumRows() { 
		return dimensions.num_rows;
	}
	
	int getNumColumns() { 
		return dimensions.num_cols;
	}
	
    void clearOccupants() {
        memset(occupants, 0, sizeof(occupants));
    }

	bool isOccupied(int row, int col) {
		return occupants[row][col] != NULL;
	}

	bool isUnoccupied(int row, int col) {
		return occupants[row][col] == NULL;
	}

	void setOccupant(Occupant * pOccupant, int row, int col) {
		occupants[row][col] = pOccupant;
	}

	Occupant * getOccupant(int row, int col) {
		return occupants[row][col];
	}

	void clearRovers() {
		for (int row = 0; row < ROVER_FIELD_ROWS; row++) {
			for (int col = 0; col < ROVER_FIELD_COLUMNS; col++) {
				if (occupants[row][col] != NULL) {
					if (occupants[row][col]->isRover) {
						occupants[row][col] = NULL;
					}
				}
			}
		}
	}

	void clearObstacles() {
		for (int row = 0; row < ROVER_FIELD_ROWS; row++) {
			for (int col = 0; col < ROVER_FIELD_COLUMNS; col++) {
				if (occupants[row][col] != NULL) {
					if (occupants[row][col]->isRover == false) {
						occupants[row][col] = NULL;
					}
				}
			}
		}
	}

	bool isObstacle(int row, int col) {
		return (occupants[row][col] != NULL) && (occupants[row][col]->isRover == false);
	}

	bool isRover(int row, int col) {
		return (occupants[row][col] != NULL) && (occupants[row][col]->isRover == true);
	}

	Rover * getRover(int row, int col) { 
		if ((occupants[row][col] != NULL) && (occupants[row][col]->isRover == true)) {
			return static_cast<Rover*>(occupants[row][col]);
		}
		return NULL; // not occupied or not rover 
	}

	Obstacle * getObstacle(int row, int col) { 
		if ((occupants[row][col] != NULL) && (occupants[row][col]->isRover == false)) {
			return static_cast<Obstacle*>(occupants[row][col]);
		}
		return NULL; // not occupied or not obstacle 
	}
	
}; 

struct RoverState : ListNode { 
	int row; 
	int col; 
	int birthday;	
	RoverType type; 
	SpeedType speed;
	int collisionCount;

	void initialize(Rover * pRover) {
		row = pRover->pos_row;
		col = pRover->pos_col;
		birthday = pRover->birthday;
		type = pRover->roverType;
		speed = pRover->speed;
		collisionCount = pRover->collisionCount;
	}

	void populateRover(Rover * pRover) {
		pRover->pos_row = row;
		pRover->pos_col = col;
		pRover->birthday = birthday;
		pRover->roverType = type;
		pRover->speed =speed;
		pRover->collisionCount = collisionCount;
	}
};

struct ObstacleState : ListNode { 
	int row; 
	int col; 
	int birthday;	
	ObstacleType type; 
	int hitCount;

	void initialize(Obstacle * pObstacle) {
		row = pObstacle->pos_row;
		col = pObstacle->pos_col;
		birthday = pObstacle->birthday;	
		type = pObstacle->obstacleType;
		hitCount = pObstacle->hitCount;
	}

	void populateObstacle(Obstacle * pObstacle) {
		pObstacle->pos_row = row;
		pObstacle->pos_col = col;
		pObstacle->birthday = birthday;
		pObstacle->obstacleType = type;
		pObstacle->hitCount = hitCount;
	}
};

struct RoverFieldStack;

struct RoverFieldSnapshot : ListNode {
	DoubleLinkList<RoverState> roverStates;
	DoubleLinkList<ObstacleState> obstacleStates;
	RoverFieldStack * pStack;

	RoverFieldSnapshot() {
		pStack = NULL;
	}

	void initialize(RoverFieldStack * stack) {
		pStack = stack;
	}

	void addRovers(DoubleLinkList<Rover> & rovers);
	void addObstacles(DoubleLinkList<Obstacle> & obstacles);
	void retireRovers();
	void retireObstacles();
}; 

const int MAX_STACK_SIZE = 8; 

struct RoverFieldStack {
	DoubleLinkList<RoverFieldSnapshot> roverFieldSnapshotPool;
	PtrArray<RoverFieldSnapshot> stack;

	DoubleLinkList<RoverState> roverStatePool;
	DoubleLinkList<ObstacleState> obstacleStatePool;

	RoverFieldStack() {
	}

	~RoverFieldStack() {
		stack.deleteContents();
	}

	bool isEmpty() const { 
		return stack.getNumMembers() <= 0;
	}

	bool isFull() const {
		return stack.getNumMembers() == MAX_STACK_SIZE;
	}

	int size() const { 
		return stack.getNumMembers();
	}

	int maxSize() const { 
		return MAX_STACK_SIZE;
	}

	bool push(RoverFieldSnapshot * pSnapshot) {
		if (pSnapshot && stack.getNumMembers() < MAX_STACK_SIZE) {
			stack.append(pSnapshot);
			return true;
		}
		return false; 
	}
	
	RoverFieldSnapshot * peekAt(int idx) const {
		if (idx >= 0 && idx < stack.getNumMembers()) {
			return stack.getAt(idx);
		}
		return NULL;
	}

	RoverFieldSnapshot * pop() {
		return stack.removeLast();
	}

	RoverFieldSnapshot * allocateSnapshot() {
		RoverFieldSnapshot * pSnapshot = roverFieldSnapshotPool.popFront();
		if (pSnapshot == NULL) {
			pSnapshot = new RoverFieldSnapshot;
			pSnapshot->initialize(this);
		}
		return pSnapshot;
	}

	void retire(RoverFieldSnapshot * pSnapshot) {
		if (pSnapshot) {
			pSnapshot->retireRovers();
			pSnapshot->retireObstacles();
			roverFieldSnapshotPool.pushTail(pSnapshot);
		}
	}

	RoverState * createRoverState(Rover * pRover) {
		RoverState * pRoverState = roverStatePool.popFront();
		if (pRoverState == NULL) {
			pRoverState = new RoverState;
		}
		pRoverState->initialize(pRover);
		return pRoverState;	
	}

	ObstacleState * createObstacleState(Obstacle * pObstacle) {
		ObstacleState * pObstacleState = obstacleStatePool.popFront();
		if (pObstacleState == NULL) {
			pObstacleState = new ObstacleState;
		}
		pObstacleState->initialize(pObstacle);
		return pObstacleState;	
	}

	void retireRoverState(RoverState * pRoverState) {
		if (pRoverState) {
			roverStatePool.pushTail(pRoverState);
		}
	}

	void retireObstacleState(ObstacleState * pObstacleState) {
		if (pObstacleState) {
			obstacleStatePool.pushTail(pObstacleState);
		}
	}
};

void RoverFieldSnapshot::addRovers(DoubleLinkList<Rover> & rovers) {
	DoubleLinkListIterator<Rover> rover_iter(rovers);
	while (rover_iter.hasMore()) {
		Rover * pRover = rover_iter.getNext();
		RoverState * pRoverState = pStack->createRoverState(pRover);
		roverStates.pushTail(pRoverState);
	}
}

void RoverFieldSnapshot::addObstacles(DoubleLinkList<Obstacle> & obstacles) {
	DoubleLinkListIterator<Obstacle> obstacle_iter(obstacles);
	while (obstacle_iter.hasMore()) {
		Obstacle * pObstacle = obstacle_iter.getNext();
		ObstacleState * pObstacleState = pStack->createObstacleState(pObstacle);
		obstacleStates.pushTail(pObstacleState);
	}
}

void RoverFieldSnapshot::retireRovers() {
	RoverState * pRoverState;
	while ((pRoverState = roverStates.popFront())) {
		pStack->retireRoverState(pRoverState);
	}
}

void RoverFieldSnapshot::retireObstacles() {
	ObstacleState * pObstacleState;
	while ((pObstacleState = obstacleStates.popFront())) {
		pStack->retireObstacleState(pObstacleState);
	}
}






struct WallStrikeTimer : DelayListNode { 
	WallDirection wallDirection; 
	int pitchInterval_idx;

	void initialize(WallDirection wallDirection, int pitchInterval_idx, long delaySamples) {
		this->delay = delaySamples; 
		this->wallDirection = wallDirection;
		this->pitchInterval_idx = pitchInterval_idx;
	}
};

struct OutputPitches { 
	int polyphony; // the maximum number of voices on hand

    int numPitches;
    int pitches[ MAX_POLYPHONY ];
	int minPitch;
	int maxPitch; 

	bool pitchActive[ MAX_PITCH ];

    OutputPitches() {
		polyphony = MAX_POLYPHONY;
		clear();
    }

    void clear() { 
        numPitches = 0;
        memset(pitches, 0, sizeof(pitches));
        memset(pitchActive, 0, sizeof(pitchActive));
		minPitch = 0;
		maxPitch = 0;
    }

    int getNumPitches() { 
        return numPitches;
    }

    int getPitch(int idx) { 
        return pitches[idx];
    }

	int getMinimumPitch() const { 
		return minPitch;
	}

	int getMaximumPitch() const { 
		return maxPitch;
	}

	void setPolyphony(int polyphony) {
		this->polyphony = polyphony;
	}

	void clearActivePitches() { 
        memset(pitchActive, 0, sizeof(pitchActive));
	}	

    void addPitch(int pitch) {
		if (! pitchActive[pitch]) {
			recordNewPitch(pitch);
		}
		pitchActive[pitch] = true;

//        if (numPitches < MAX_POLYPHONY) {
//			if (! pitchActive[pitch]) {
//				pitchActive[ pitch ] = true;
//	            pitches[ numPitches ] = pitch;
//				numPitches++;
//				if (pitch < minPitch || numPitches == 1) {
//					minPitch = pitch;
//				}
//				if (pitch > maxPitch) {
//					maxPitch = pitch;
//				}
//			}
//        }
    }


	void recordNewPitch(int pitch) {
		if (numPitches >= polyphony) {
			removeOldestPitch();
			assessMinMax();
		}
        pitches[ numPitches ] = pitch;
		numPitches++;
		if (pitch < minPitch || numPitches == 1) {
			minPitch = pitch;
		}
		if (pitch > maxPitch) {
			maxPitch = pitch;
		}
	}

	void removeOldestPitch() {
		// remove oldest pitches to make room for new 
		// number of remaining pitches is polyphony - 1 
		if (numPitches >= polyphony) {
			int numToDiscard = (numPitches - polyphony) + 1;
			numPitches = polyphony - 1;
			for (int k = 0; k < numPitches; k++) {
				pitches[k] = pitches[k + numToDiscard];
			}
		}
	}

	void assessMinMax() {
		minPitch = 0;
		maxPitch = 0;
		for (int k = 0; k < numPitches; k++) {
			if (pitches[k] < minPitch || k == 0) {
				minPitch = pitches[k];
			}
			if (pitches[k] > maxPitch) {
				maxPitch = pitches[k];
			}

		}
	}
};


enum CellContentType {
	CELL_EMPTY,
	CELL_ROVER,
	CELL_OBSTACLE,
	CELL_COLLISION,
	NUM_CELL_CONTENT_TYPES
};

struct UiRoverFieldCell { 
	CellContentType contentType;
	union { 
		struct {
			RoverType roverType;
			SpeedType speed;
		} rover;
		struct {
			ObstacleType obstacleType;
		} obstacle;
	} content;

	UiRoverFieldCell() {
		contentType = CELL_EMPTY;
	}

	void setRover(Rover * pRover) {
		contentType = CELL_ROVER;
		content.rover.roverType  = pRover->roverType;
		content.rover.speed = pRover->speed;
	}

	void setRoverType(RoverType roverType, SpeedType speedType) {
		contentType = CELL_ROVER;
		content.rover.roverType = roverType;
		content.rover.speed = speedType;
	}

	void setObstacle(Obstacle * pObstacle) {
		contentType = CELL_OBSTACLE;
		content.obstacle.obstacleType = pObstacle->obstacleType;
	}

	void setObstacleType(ObstacleType obstacleType) {
		contentType = CELL_OBSTACLE;
		content.obstacle.obstacleType = obstacleType;
	}
	
	void setCollision() {
		contentType = CELL_COLLISION;		
	}

	void setEmpty() {
		contentType = CELL_EMPTY;
	}

	bool isRover() const { 
		return contentType == CELL_ROVER;
	}

	bool isObstacle() const { 
		return contentType == CELL_OBSTACLE;
	}

	bool isCollision() const { 
		return contentType == CELL_COLLISION;
	}

	bool isEmpty() const { 
		return contentType == CELL_EMPTY;
	}

	RoverType getRoverType() const { 
		return content.rover.roverType;
	}

	SpeedType getRoverSpeed() const { 
		return content.rover.speed;
	}

	ObstacleType getObstacleType() const { 
		return content.obstacle.obstacleType;
	}

};

struct UiWallCell { 
	CellContentType contentType;
	int pitch;

	UiWallCell() {
		contentType = CELL_EMPTY;
		pitch = 0;
	}

	void setEmpty() {
		contentType = CELL_EMPTY;
	}

	void setCollision() { 
		contentType = CELL_COLLISION;
	}

	bool isCollision() { 
		return contentType == CELL_COLLISION;
	}

	void setPitch(int pitch) {
		this->pitch = pitch;
	}
};

struct UiWall { 
	UiWallCell blocks[ ROVER_FIELD_DIMENSION ];
	int lowestPitch;
	int highestPitch;

	UiWall() { 
		lowestPitch = 36; // C
		highestPitch = 36; // C
		clear();
	}

	void clear() {
		for (int idx = 0; idx < ROVER_FIELD_DIMENSION; idx++) {
			blocks[idx].setEmpty();
		}
	}
};

struct UiMatrix {
	int numRowsActive;
	int numColumnsActive;
	UiRoverFieldCell cells[ ROVER_FIELD_ROWS ][ ROVER_FIELD_COLUMNS ];
	UiWall walls[ NUM_WALLS ];

	UiMatrix() {
		numRowsActive = ROVER_FIELD_ROWS;
		numColumnsActive = ROVER_FIELD_COLUMNS;
	}

	void clear() {
		for (int row = 0; row < ROVER_FIELD_ROWS; row++) {
			for (int col = 0; col < ROVER_FIELD_COLUMNS; col++) {
				cells[row][col].setEmpty();
			}
		}
		for (int k = 0; k < NUM_WALLS; k++) {
			walls[k].clear();
		}
	}
};


struct OccupantSelector {
	bool rover_selected;
	RoverType roverType;
	SpeedType speed;
	ObstacleType obstacleType;

	OccupantSelector() {
		rover_selected = true;
		roverType = ROVER_NORTH;
		speed = SPEED_NORMAL;
		obstacleType = OBSTACLE_MIRROR_HORIZONTAL;
	}

	bool isRover() const {
		return rover_selected;
	}

	bool isObstacle() const {
		return (!rover_selected);
	}

	RoverType getRoverType() const {
		return roverType;
	}

	ObstacleType getObstacleType() const {
		return obstacleType;
	}

	void selectRoverType(RoverType roverType) {
		this->roverType = roverType;
		rover_selected = true;
	}

	void selectObstacleType(ObstacleType obstacleType) {
		this->obstacleType = obstacleType;
		rover_selected = false;
	}
};


enum RoverActionType { 
	ROVER_ACTION_ADD,
	ROVER_ACTION_REMOVE,
	NUM_ROVER_ACTIONS
};

struct RoverAction : ListNode { 
	RoverActionType actionType; 
	RoverType roverType;
	int row;
	int column;

	RoverAction() {
		actionType = ROVER_ACTION_ADD;
		roverType = ROVER_NORTH;
		row = 0;
		column = 0;
	}

	void initialize(RoverActionType actionType, RoverType roverType, int row, int column) {
		this->actionType = actionType;
		this->roverType = roverType;
		this->row = row; 
		this->column = column;
	}

	int getRow() { 
		return row;;
	}

	int getColumn() { 
		return column;
	}
};

enum ObstacleActionType { 
	OBSTACLE_ACTION_ADD,
	OBSTACLE_ACTION_REMOVE,
	NUM_OBSTACLE_ACTIONS
};

struct ObstacleAction : ListNode { 
	ObstacleActionType actionType; 
	ObstacleType obstacleType;
	int row;
	int column;

	ObstacleAction() {
		actionType = OBSTACLE_ACTION_ADD;
		obstacleType = OBSTACLE_MIRROR_HORIZONTAL;
		row = 0;
		column = 0;
	}

	void initialize(ObstacleActionType actionType, ObstacleType obstacleType, int row, int column) {
		this->actionType = actionType;
		this->obstacleType = obstacleType;
		this->row = row; 
		this->column = column;
	}

	int getRow() { 
		return row;;
	}

	int getColumn() { 
		return column;
	}
};


struct CollisionTracker {
	int roverCount[ ROVER_FIELD_ROWS * ROVER_FIELD_COLUMNS ];	// index by rover cell id 
	int numCollisions; 

	CollisionTracker() {
		clear();
	}

	void clear() {
		memset(roverCount, 0, sizeof(roverCount));
		numCollisions = 0;
	}

	void recordRoverPosition(Rover * pRover) {
		int cell_id = pRover->getCellId();
		roverCount[cell_id]++;
		if (roverCount[cell_id] == 2) {
			numCollisions ++;
		}
	}

	bool isCollidedRover(Rover * pRover) const {
		int cell_id = pRover->getCellId();
		return roverCount[cell_id] > 1;
	}

	int getNumCollisions() const { 
		return numCollisions;
	}
};


// TODO: move to separate file 
struct ClockWatcher {
	SampleRateCalculator  sampleRateCalculator;
	bool externalClockConnected; 
	float externalClock_cv;	

	float samplesSinceExternalLeadingEdge;
	float samplesSinceClockPulse;

	ClockWatcher() {
		samplesSinceClockPulse = 0.f;
		externalClockConnected = false;
		externalClock_cv = 0.5f; 	
		samplesSinceExternalLeadingEdge = 0.f;
	}

	void setBeatsPerMinute(float beatsPerMinute) {
		sampleRateCalculator.setBpm(beatsPerMinute);
	}

	void setSampleRate(float sampleRate) {
		sampleRateCalculator.setSampleRate(sampleRate);
	}

	float getBeatsPerMinute() const {
		return sampleRateCalculator.bpm;
	}

	float getSamplesPerBeat() const {
		return sampleRateCalculator.getSamplesPerBeat();
	}

	float getSampleRate() const { 
		return sampleRateCalculator.sampleRate;
	}

	bool isConnectedToExternalClock() const {
		return externalClockConnected;
	}

	void reset() { 
/////		clockTrigger.reset();  // TODO: remove this method?
	}

	bool isLeadingEdge() const {
		return samplesSinceClockPulse <= 0.f;
	}

	void tick(Input & input) {

		// Update BPM based on leading edge of external clock pulse 

		samplesSinceClockPulse ++;

		if (input.isConnected()) {
			samplesSinceExternalLeadingEdge ++;

			//float clock_cv = rescale(input.getVoltage(), 0.1f, 9.9f, 0.f, 1.f);
			float clock_cv = input.getVoltage();
			// DEBUG("ClockWatcher: External Clock clock_cv %f, samples since %f", clock_cv, samplesSinceExternalLeadingEdge);

			if (externalClock_cv <= 0.f && clock_cv >= 10.f) {
				//DEBUG("ClockWatcher: External Clock EDGE DETECTED: clock_cv %f, samples since external leading edge %f", clock_cv, samplesSinceExternalLeadingEdge);
				// Transition low to high 
				if (externalClockConnected) {
					// have detected at least one full cycle 
					// DEBUG("ClockWatcher: External Clock SET BPM clock_cv %f, samples since %f", clock_cv, samplesSinceExternalLeadingEdge);
					sampleRateCalculator.setBpmFromNumSamples(samplesSinceExternalLeadingEdge);
					samplesSinceClockPulse = sampleRateCalculator.getSamplesPerBeat();
					//DEBUG("ClockWatcher: Computed BPM as %f, samplesPerBeat %f", sampleRateCalculator.bpm, sampleRateCalculator.getSamplesPerBeat());
					//DEBUG("   samples since external leading edge = %f", samplesSinceExternalLeadingEdge);
				}
				samplesSinceExternalLeadingEdge = 0.f;
				externalClockConnected = true;
			}
			externalClock_cv = clock_cv;
		}
		else {
			samplesSinceExternalLeadingEdge = 0.f;
			externalClockConnected = false;
		}

		float overshoot = samplesSinceClockPulse - sampleRateCalculator.getSamplesPerBeat();
		if (overshoot >= 0) {
			samplesSinceClockPulse = overshoot; // capture the overshoot 
		}
	}

	void tick() {
		externalClockConnected = false;
		samplesSinceClockPulse += 1.f;
		float overshoot = samplesSinceClockPulse - sampleRateCalculator.getSamplesPerBeat();
		if (overshoot >= 0) {
			samplesSinceClockPulse = overshoot; // capture the overshoot 
		}
	}
};

struct SampleCountingClock {
	FloatCounter sampleCounter;
	float stepSize;    // number of ticks per cycle
	float pulseWidth;  // 0.5 = even high,low, 0.66 = swing
	float highSideSamples; // number of samples in the High portion of the cycle 
	bool isHigh;       // true while clock is in high portion
	bool leadingEdge;  // true when clock goes high (begin new step)
	bool trailingEdge; // true when clock goes low ("half-way" point based on pulseWidth)

	SampleCountingClock() {
		stepSize = 44100.f;
		pulseWidth = 0.5f;
		highSideSamples = stepSize * pulseWidth;
		isHigh = false;
		leadingEdge = true;
		trailingEdge = false;
		computePosition();
	}

	void setStepSize(float stepSize) {
		if (this->stepSize != stepSize) {
			this->stepSize = stepSize;
			computePosition();
		}
	}

	// sets % of time for the high side of the cycle
	// values of 0 and 1 are nonsensical
	//   0.5 is even igh/low sides 
	//   0.66666 is swing division (2/3)
	void setPulseWidth(float pulseWidth) {
		if (this->pulseWidth != pulseWidth) {
			this->pulseWidth = pulseWidth;
			computePosition();
		}
	}

	bool isLeadingEdge() const {
		return leadingEdge;
	}

	bool isTrailingEdge() const {
		return trailingEdge;
	}

	bool isEdge() const {
		return leadingEdge || trailingEdge;
	}

	float getHighSideLength() const {
		return highSideSamples;
	}

	float getLowSideLength() const {
		return stepSize - highSideSamples;
	}

	void sync() {  // begin new cycle
		sampleCounter.reset();
		isHigh = true;
		leadingEdge = true;
		trailingEdge = false;
	}

	bool tick() { 
		leadingEdge = sampleCounter.tick();
		trailingEdge = isHigh && (sampleCounter.getCount() >= highSideSamples);
		if (leadingEdge) {
			isHigh = true;
		}
		else if (trailingEdge) {
			isHigh = false;
		}
		return leadingEdge;
	}

	void computePosition() {
		sampleCounter.setTarget(stepSize);
		float sampleCount = sampleCounter.getCount();
		highSideSamples = stepSize * pulseWidth;
		leadingEdge = (sampleCount >= stepSize);
		trailingEdge = isHigh && (sampleCount >= highSideSamples);
		if (leadingEdge) {
			sampleCount = 0;
			isHigh = true;
		}
		else if (trailingEdge) {
			isHigh = false;
		}
	}
};

enum OutputMode { 
	OUTPUT_MODE_MINIMUM, 
	OUTPUT_MODE_MAXIMUM, 
	OUTPUT_MODE_POLY,
	NUM_OUTPUT_MODES
};

struct Traveler : Module {
	enum ParamIds {
		RUN_RATE_PARAM,
		WALL_PROBABILITY_PARAM,
		OBSTACLE_PROBABILITY_PARAM,
		NOTE_PROBABILITY_PARAM,
		STEADY_PROBABILITY_PARAM,
		CLEAR_OBSTACLES_PARAM,
		RUN_BUTTON_PARAM,
		REVERSE_BUTTON_PARAM,
		WALL_PROBABILITY_ATTENUVERTER_PARAM,
		OBSTACLE_PROBABILITY_ATTENUVERTER_PARAM,
		NOTE_PROBABILITY_ATTENUVERTER_PARAM,
		STEADY_PROBABILITY_ATTENUVERTER_PARAM,
		CLEAR_ROVERS_PARAM,
		UNDO_PARAM,
		NOTE_LENGTH_ATTENUVERTER_PARAM,
		NOTE_LENGTH_PARAM,
		SCALE_INVERT_WEST_PARAM,
		SCALE_INVERT_NORTH_PARAM,
		SCALE_INVERT_SOUTH_PARAM,
		SCALE_INVERT_EAST_PARAM,
		ROW_COUNT_ATTENUVERTER_PARAM,
		ROW_COUNT_PARAM,
		HERD_BUTTON_PARAM,
		SHIFT_NORTH_PARAM,
		SHIFT_SOUTH_PARAM,
		SHIFT_EAST_PARAM,
		SHIFT_WEST_PARAM,
		COLUMN_COUNT_ATTENUVERTER_PARAM,
		COLUMN_COUNT_PARAM,
		RESET_WALLS_PARAM,
		ROTATE_NORTH_PARAM,
		ROTATE_SOUTH_PARAM,
		ROTATE_EAST_PARAM,
		ROTATE_WEST_PARAM,
		CLIMB_NORTH_PARAM,
		CLIMB_SOUTH_PARAM,
		CLIMB_EAST_PARAM,
		CLIMB_WEST_PARAM,
		OUTPUT_MODE_PARAM,
		SELECT_SCALE_1_BUTTON_PARAM,
		SCALE_1_ROOT_PARAM,
		SELECT_SCALE_2_BUTTON_PARAM,
		SCALE_2_ROOT_PARAM,
		MUTE_NORTH_PARAM,
		MUTE_SOUTH_PARAM,
		MUTE_EAST_PARAM,
		MUTE_WEST_PARAM,
		SELECT_SCALE_3_BUTTON_PARAM,
		SCALE_3_ROOT_PARAM,
		SELECT_SCALE_4_BUTTON_PARAM,
		SCALE_4_ROOT_PARAM,
		ALT_SPEED_MODE_PARAM,
		NUM_PARAMS
	};
	enum InputIds {
		CLOCK_CV_INPUT,
		RUN_CV_INPUT,
		REVERSE_CV_INPUT,
		WALL_PROBABILITY_CV_INPUT,
		OBSTACLE_PROBABILITY_CV_INPUT,
		NOTE_PROBABILITY_CV_INPUT,
		STEADY_PROBABILITY_CV_INPUT,
		NOTE_LENGTH_CV_INPUT,
		ROW_COUNT_CV_INPUT,
		COLUMN_COUNT_CV_INPUT,
		SCALE_SELECTOR_CV_INPUT,
		NUM_INPUTS
	};
	enum OutputIds {
		GATE_NORTH_OUTPUT,
		GATE_SOUTH_OUTPUT,
		GATE_EAST_OUTPUT,
		GATE_WEST_OUTPUT,
		GATE_POLY_OUTPUT,
		VOCT_NORTH_OUTPUT,
		VOCT_SOUTH_OUTPUT,
		VOCT_EAST_OUTPUT,
		VOCT_WEST_OUTPUT,
		VOCT_POLY_OUTPUT,
		NUM_OUTPUTS
	};
	enum LightIds {
		RUN_LED_LIGHT,
		REVERSE_LED_LIGHT,
		SCALE_INVERT_WEST_LED_LIGHT,
		SCALE_INVERT_NORTH_LED_LIGHT,
		SCALE_INVERT_SOUTH_LED_LIGHT,
		SCALE_INVERT_EAST_LED_LIGHT,
		HERD_LED_LIGHT,
		SCALE_1_LED_LIGHT,
		OUTPUT_MODE_MINIMUM_LIGHT,
		SCALE_2_LED_LIGHT,
		OUTPUT_MODE_MAXIMUM_LIGHT,
		MUTE_NORTH_LED_LIGHT,
		MUTE_SOUTH_LED_LIGHT,
		MUTE_EAST_LED_LIGHT,
		MUTE_WEST_LED_LIGHT,
		OUTPUT_MODE_POLY_LIGHT,
		SCALE_3_LED_LIGHT,
		SCALE_4_LED_LIGHT,
		ALT_SPEED_HALF_LIGHT,
		ALT_SPEED_DOUBLE_LIGHT,
		NUM_LIGHTS
	};


	ClockWatcher clock;
	float activeSamplesPerBeat; // to detect changes in clock rate 
	float activeBpm; 

	DoubleLinkList<Rover> activeRovers;
	DoubleLinkList<Rover> roverPool;
	

	DoubleLinkList<Obstacle> activeObstacles;
	DoubleLinkList<Obstacle> obstaclePool;

	PtrArray<Rover> displacedRovers; 

	Wall northWall{WALL_NORTH}; 
	Wall eastWall{WALL_EAST}; 
	Wall southWall{WALL_SOUTH}; 
	Wall westWall{WALL_WEST}; 
	Wall * walls[NUM_WALLS] = {     // TODO: position must match wall[DIRECTION]. Consider using Wall walls[4]; and assigning direction at module constructor 
		&northWall, 
		&eastWall,
		&southWall,
		&westWall
	};

	struct Param {
		int paramId;
	};

	struct ParamWithCv {
		int paramId;
		int cvId;
	};

	struct ParamWithLight {
		int paramId;
		int lightId;
	};


	struct WallParams {
		Param shift;
		Param rotate;
		Param climb;
		ParamWithLight ascending;
		ParamWithLight mute;
	};

	WallParams wallParamsTable[ NUM_WALLS ]= {
		{
			{ SHIFT_NORTH_PARAM  },
			{ ROTATE_NORTH_PARAM },
			{ CLIMB_NORTH_PARAM  },
			{ SCALE_INVERT_NORTH_PARAM, SCALE_INVERT_NORTH_LED_LIGHT },
			{ MUTE_NORTH_PARAM,         MUTE_NORTH_LED_LIGHT },
		},
		{ 
			{ SHIFT_EAST_PARAM  },
			{ ROTATE_EAST_PARAM },
			{ CLIMB_EAST_PARAM  },
			{ SCALE_INVERT_EAST_PARAM, SCALE_INVERT_EAST_LED_LIGHT },
			{ MUTE_EAST_PARAM,         MUTE_EAST_LED_LIGHT },
		}, 
		{ 
			{ SHIFT_SOUTH_PARAM  },
			{ ROTATE_SOUTH_PARAM },
			{ CLIMB_SOUTH_PARAM  },
			{ SCALE_INVERT_SOUTH_PARAM, SCALE_INVERT_SOUTH_LED_LIGHT },
			{ MUTE_SOUTH_PARAM,         MUTE_SOUTH_LED_LIGHT },
		}, 
		{ 
			{ SHIFT_WEST_PARAM  },
			{ ROTATE_WEST_PARAM },
			{ CLIMB_WEST_PARAM  },
			{ SCALE_INVERT_WEST_PARAM, SCALE_INVERT_WEST_LED_LIGHT },
			{ MUTE_WEST_PARAM,         MUTE_WEST_LED_LIGHT },
		}
	};

	void setWallAscending(Wall & wall, bool beAscending) {
		wall.setAscending(beAscending);
		int lightId = wallParamsTable[ wall.direction ].ascending.lightId;
		lights[ lightId ].setBrightness( (!beAscending) );	
	}

	void setWallMuted(Wall & wall, bool beMuted) {
		wall.setMuted(beMuted);
		int lightId = wallParamsTable[ wall.direction ].mute.lightId;
		lights[ lightId ].setBrightness( beMuted );	
	}

	
	dsp::SchmittTrigger scaleDirectionTriggers[ NUM_WALLS ];
	dsp::SchmittTrigger wallMuteTriggers[ NUM_WALLS ];
	dsp::SchmittTrigger wallResetTrigger;

	Matrix matrix; 

	CollisionTracker collisionTracker;

	// Action Queue shared by main thread and UI thread to add rovers, etc 
	ThreadSafeDoubleLinkList<RoverAction> roverActionQueue;
	ThreadSafeDoubleLinkList<RoverAction> roverActionPool;

	ThreadSafeDoubleLinkList<ObstacleAction> obstacleActionQueue;
	ThreadSafeDoubleLinkList<ObstacleAction> obstacleActionPool;

	// Timed actions to close the gate on specific wall strikes 
	DelayList<WallStrikeTimer> wallStrikeTimerList;
	DoubleLinkList<WallStrikeTimer> wallStrikeTimerPool;

	dsp::SchmittTrigger clearRoversTrigger;
	dsp::SchmittTrigger clearObstaclesTrigger;
	dsp::SchmittTrigger undoTrigger;
	dsp::SchmittTrigger reverseTrigger;
	dsp::SchmittTrigger runTrigger;
	dsp::SchmittTrigger herdTrigger;
	bool isPaused; // true if the rover motion is paused, false if running  
	bool isReverse;
	bool isHerding; 

	int activeScaleIdx; 
	int activeRootNote;


	enum ScaleId { 
		SCALE_1, 
		SCALE_2, 
		SCALE_3, 
		SCALE_4, 
		NUM_SCALES
	};

	struct ScaleDefinition {
		ScaleTemplate<ROVER_FIELD_DIMENSION> scaleTemplate;
		int rootNote;
		std::string filepath;

		ScaleDefinition() {
			rootNote = DFLT_ROOT_NOTE;
			filepath = FILE_NOT_SELECTED_STRING;
		}
	};

    struct ScaleParameters {
		ScaleId scaleId;
		int rootNoteParamId; 
		int scaleSelectParamId;
		int scaleActiveLedId;
	}; 

	const ScaleParameters scaleParams[ NUM_SCALES ] = {
		{ SCALE_1, SCALE_1_ROOT_PARAM, SELECT_SCALE_1_BUTTON_PARAM, SCALE_1_LED_LIGHT },
		{ SCALE_2, SCALE_2_ROOT_PARAM, SELECT_SCALE_2_BUTTON_PARAM, SCALE_2_LED_LIGHT },
		{ SCALE_3, SCALE_3_ROOT_PARAM, SELECT_SCALE_3_BUTTON_PARAM, SCALE_3_LED_LIGHT },
		{ SCALE_4, SCALE_4_ROOT_PARAM, SELECT_SCALE_4_BUTTON_PARAM, SCALE_4_LED_LIGHT },
	}; 

	ScaleDefinition scales[ NUM_SCALES ];

	dsp::SchmittTrigger scaleTriggers[ NUM_SCALES ];
	ScaleId activeScaleId;

	void selectNextScale() { 
		setActiveScaleId( (ScaleId) ((int(activeScaleId) + 1) % NUM_SCALES) );
	}

	void setActiveScaleId(ScaleId scaleId) {
		activeScaleId = scaleId; 
		for (int k = 0; k < NUM_SCALES; k++) {
			lights[  scaleParams[k].scaleActiveLedId ].setBrightness( activeScaleId == scaleParams[k].scaleId );
		}
		applyScaleChange(scaleId);
	}

	void setRootNote(ScaleId scaleId, int rootNote) {
		scales[scaleId].rootNote = rootNote;
		applyScaleChange(scaleId);
	}

	void applyScaleChange(ScaleId scaleId) {
		if (scaleId == activeScaleId) {
			for (int k = 0; k < NUM_WALLS; k++) {
				// TODO: combine to one method: setScale(template, rootNote)
				walls[k]->setScaleTemplate(scales[activeScaleId].scaleTemplate);
				walls[k]->setRootNote(scales[activeScaleId].rootNote);
	//			walls[k]->setScaleExtendDirection(SCALE_EXTEND_REFLECT);
				walls[k]->setScaleExtendDirection(SCALE_EXTEND_UPWARD);
			}
			redrawRequired = true;
		}
	}

	void setScaleFile(ScaleId scaleId, std::string const& path) {

		// DEBUG("Load Scale Template[%d] from %s", scaleId, path.c_str());
		bool success = importScaleTemplate(scales[scaleId], path);
		if (success) {
			applyScaleChange(scaleId);
		}

	}


	bool importScaleTemplate(ScaleDefinition & scaleDefinition, std::string path) {

		if (scaleDefinition.filepath == path) 
			return false; // update not required

		if (path == "") {
			scaleDefinition.filepath = FILE_NOT_SELECTED_STRING;
			return true; // update required 		
		} else {
			bool loaded = scaleDefinition.scaleTemplate.loadFromFile(path);
			if (loaded) {
				scaleDefinition.filepath = path;
				// DEBUG("SUCCESS: name '%s', numPitches %d", scaleDefinition.scaleTemplate.name.c_str(), scaleDefinition.scaleTemplate.numPitches);
			}
			return loaded;
		}

	}

	// called from local and UI to add rover actions
	// process() method dequeues and performs actions
	void enqueueRoverAction(RoverActionType actionType, RoverType roverType, int row, int column) {
		RoverAction * pRoverAction = createRoverAction(actionType, roverType, row, column);
		roverActionQueue.enqueue(pRoverAction);
	}

	void enqueueObstacleAction(ObstacleActionType actionType, ObstacleType obstacleType, int row, int column) {
		ObstacleAction * pObstacleAction = createObstacleAction(actionType, obstacleType, row, column);
		obstacleActionQueue.enqueue(pObstacleAction);
	}

	OutputPitches polyOutputPitches; 
	OutputPitches wallOutputPitches[NUM_WALLS];

// TODO: NOT YET 
//	template<class T>
//	struct LightMap {
//		int lightId;
//		T value;
//	};
//
//	template <class T, int SIZE>
//	struct TriggeredCycler {
//		T value;
//		LightMap<T> & lightMap[SIZE];
//		SchmidtTrigger ... 
//		lights[] 
//
//		TriggeredCycler(LightMap<T> lightTable) : lightMap(lightTable) {
//			value = (T) 0;
//		}
//
//		void setValue(T value) {
//			this->value = value;
//		}
//	}; 

	dsp::SchmittTrigger outputModeTrigger;
	OutputMode outputMode; 
	int activePolyphony; 
	void setActivePolyphony(int polyphony) {
		// DEBUG("-- SET Active Polyphony: %d", polyphony);
		activePolyphony = clamp(polyphony, 1, MAX_POLYPHONY);
		activePolyphony = polyphony;
		setPolyphony(activePolyphony);
	}

	void selectNextOutputMode() { 
		setOutputMode( (OutputMode) ((int(outputMode) + 1) % NUM_OUTPUT_MODES) );
	}

	void setOutputMode(OutputMode mode) {
		outputMode = mode; 
		lights[OUTPUT_MODE_MINIMUM_LIGHT].setBrightness( outputMode == OUTPUT_MODE_MINIMUM );
		lights[OUTPUT_MODE_MAXIMUM_LIGHT].setBrightness( outputMode == OUTPUT_MODE_MAXIMUM );
		lights[OUTPUT_MODE_POLY_LIGHT].setBrightness( outputMode == OUTPUT_MODE_POLY );

		int maxVoices = (outputMode == OUTPUT_MODE_POLY ? activePolyphony : 1);
		setPolyphony(maxVoices);
	}

	void setPaused(bool bePaused) {
		isPaused = bePaused;
		lights[RUN_LED_LIGHT].setBrightness( !isPaused );
	}

	void setReverse(bool beReversed) {
		isReverse = beReversed;
		lights[REVERSE_LED_LIGHT].setBrightness( isReverse );
	}

	void setHerding(bool beHerding) {
		isHerding = beHerding;
		lights[HERD_LED_LIGHT].setBrightness( isHerding );
	}

	bool voiceUpdateRequired; // true if the set of output voices changed
                              // due to new strike or gate timeout

	// Double buffered UI matrix, audio thread produces, ui thread consumes 
	// Audio rate thread writes to pProducer and increments producerCount to indicate update is ready 
	// UI thread reads from consumers and sets consumer count to indicate update is complete
	// Audio thread swaps pointers when producing and UI has consumed previous production (consumerCount == producerCount) 
	UiMatrix uiMatrix[2]; 
	UiMatrix * pProducer = &uiMatrix[0];
	UiMatrix * pConsumer = &uiMatrix[1];
	int producerCount = 0;
	int consumerCount = 0;
	bool uiUpdatePending = false;

	bool redrawRequired; // set true if UI needs redraw


	void markUiDirty() {
		populateUiMatrix();
		uiUpdatePending = true;
		pushUiUpdates(); 
	}

	void pushUiUpdates() { 
		if (uiUpdatePending) {
			// If UI is caught up then swap ptrs
			if (consumerCount == producerCount) {
				UiMatrix * pTemp = pProducer;
				pProducer = pConsumer;
				pConsumer = pTemp;
				producerCount ++;
				uiUpdatePending = false;
			}
		}
	}

	void populateUiMatrix() {

		int numRows = matrix.dimensions.num_rows;
		int numColumns = matrix.dimensions.num_cols;

		pProducer->numRowsActive = numRows;
		pProducer->numColumnsActive = numColumns;
		pProducer->clear();

		DoubleLinkListIterator<Rover> iter(activeRovers);
		while (iter.hasMore()) {
			Rover * pRover = iter.getNext();
			if (pRover->isColliding()) {
				pProducer->cells[ pRover->pos_row ][ pRover->pos_col ].setCollision();
			} else {
				pProducer->cells[ pRover->pos_row ][ pRover->pos_col ].setRover(pRover);
			}
		}

		DoubleLinkListIterator<Obstacle> obstacle_iter(activeObstacles);
		while (obstacle_iter.hasMore()) {
			Obstacle * pObstacle = obstacle_iter.getNext();
			pProducer->cells[ pObstacle->pos_row ][ pObstacle->pos_col ].setObstacle(pObstacle);
		}

		for (int k = 0; k < NUM_WALLS; k++) {
			Wall * pWall = walls[k];
			pProducer->walls[k].lowestPitch = pWall->getLowestPitch();
			pProducer->walls[k].highestPitch = pWall->getHighestPitch();
			for (int block_idx = 0; block_idx < ROVER_FIELD_DIMENSION; block_idx++) {

				if (pWall->isStrikeActive(block_idx)) {
					pProducer->walls[k].blocks[block_idx].setCollision();
				} else { 
					pProducer->walls[k].blocks[block_idx].setEmpty();
				}
				pProducer->walls[k].blocks[block_idx].pitch = pWall->getPitchForBlock(block_idx);
			}
		}
	}

	// Speed for ALT speed rovers 
	enum SpeedMode { 
		SPEED_MODE_HALF,
		SPEED_MODE_DOUBLE,
		NUM_SPEED_MODES
	};

	SampleCountingClock  roverStepClock; 
	bool roverStepClockEven;   // even/odd leading edge toggle to perfrom half speed movements
	dsp::SchmittTrigger alternateSpeedModeTrigger;
	SpeedMode alternateSpeedMode;

	void selectNextAlternateSpeedMode() {
		setAlternateSpeedMode( (SpeedMode) ((int(alternateSpeedMode) + 1) % NUM_SPEED_MODES) );
	}

	void setAlternateSpeedMode(SpeedMode mode) {
		alternateSpeedMode = mode;
		lights[ ALT_SPEED_HALF_LIGHT ].setBrightness( mode == SpeedMode::SPEED_MODE_HALF );
		lights[ ALT_SPEED_DOUBLE_LIGHT ].setBrightness( mode == SpeedMode::SPEED_MODE_DOUBLE );
	}

	SampleCountingClock  paramUpdateClock; // update the parameters per this downsampling clock rate  



	// EXPERIMENT 
	// Expander module 
	// TravelerExpanderMessage rightMessages[2];

	// EXPERIMENT 
	// Stack of preserved patterns 
	RoverFieldStack roverFieldStack;
	int activeStackIndex = -1; 
	// up trigger
	// down trigger
	// push/save trigger 
	// reset triggers

	float activeNoteLength;

	long wallStrikeHighlightSamples;
	long collisionHighlightSamples;

	FastRandom random; // pseudo/approx random number generator 

	float obstacleProbability; 
	float wallProbability;
	float noteProbability;
	float steadyProbability;

	// For identifying what type of occupant to drop into rover field on mouse clicks
	OccupantSelector occupantSelector;

	Traveler() {
		config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
		configParam(RUN_RATE_PARAM, 0.f, 1.f, 0.5f, "BPM");
		configParam(WALL_PROBABILITY_PARAM, 0.f, 1.f, 1.f, "Wall Probability");
		configParam(OBSTACLE_PROBABILITY_PARAM, 0.f, 1.f, 1.f, "Obstacle Probability");
		configParam(NOTE_PROBABILITY_PARAM, 0.f, 1.f, 1.f, "Note Probability");
		configParam(STEADY_PROBABILITY_PARAM, 0.f, 1.f, 1.f, "Steady Probability");
		configParam(CLEAR_OBSTACLES_PARAM, 0.f, 1.f, 0.f, "Clear Obstacles");
		configParam(RUN_BUTTON_PARAM, 0.f, 1.f, 1.f, "Run");
		configParam(REVERSE_BUTTON_PARAM, 0.f, 1.f, 0.f, "Reverse");
		configParam(WALL_PROBABILITY_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Wall Prob AV");
		configParam(OBSTACLE_PROBABILITY_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Obs Prob AV");
		configParam(NOTE_PROBABILITY_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Note Prob AV");
		configParam(STEADY_PROBABILITY_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Steady Prob AV");
		configParam(CLEAR_ROVERS_PARAM, 0.f, 1.f, 0.f, "Clear Rovers");
		configParam(UNDO_PARAM, 0.f, 1.f, 0.f, "Clear Newest");
		configParam(NOTE_LENGTH_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Note Len AV");
		configParam(NOTE_LENGTH_PARAM, 0.f, 1.f, 0.5f, "Note Length");
		configParam(SCALE_INVERT_WEST_PARAM, 0.f, 1.f, 0.f, "Scale Invert West");
		configParam(SCALE_INVERT_NORTH_PARAM, 0.f, 1.f, 0.f, "Scale Invert North");
		configParam(SCALE_INVERT_SOUTH_PARAM, 0.f, 1.f, 0.f, "Scale Invert South");
		configParam(SCALE_INVERT_EAST_PARAM, 0.f, 1.f, 0.f, "Scale Invert East");
		configParam(ROW_COUNT_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Row Count AV");
		configParam(ROW_COUNT_PARAM, 0.f, 1.f, 1.f, "Row Count");
		configParam(HERD_BUTTON_PARAM, 0.f, 1.f, 1.f, "Herd");
		configParam(SHIFT_NORTH_PARAM, -1.f, 1.f, 0.f, "Rotate North");
		configParam(SHIFT_SOUTH_PARAM, -1.f, 1.f, 0.f, "Rotate South");
		configParam(SHIFT_EAST_PARAM, -1.f, 1.f, 0.f, "Rotate East");
		configParam(SHIFT_WEST_PARAM, -1.f, 1.f, 0.f, "Rotate West");
		configParam(COLUMN_COUNT_ATTENUVERTER_PARAM, -1.f, 1.f, 0.f, "Col Count AV");
		configParam(COLUMN_COUNT_PARAM, 0.f, 1.f, 1.f, "Column Count");
		configParam(RESET_WALLS_PARAM, 0.f, 1.f, 0.f, "Home");
		configParam(ROTATE_NORTH_PARAM, -1.f, 1.f, 0.f, "Autorotate North");
		configParam(ROTATE_SOUTH_PARAM, -1.f, 1.f, 0.f, "Autorotate South");
		configParam(ROTATE_EAST_PARAM, -1.f, 1.f, 0.f, "Autorotate East");
		configParam(ROTATE_WEST_PARAM, -1.f, 1.f, 0.f, "Autorotate West");
		configParam(CLIMB_NORTH_PARAM, -1.f, 1.f, 0.f, "Climb North");
		configParam(CLIMB_SOUTH_PARAM, -1.f, 1.f, 0.f, "Climb South");
		configParam(CLIMB_EAST_PARAM, -1.f, 1.f, 0.f, "Climb East");
		configParam(CLIMB_WEST_PARAM, -1.f, 1.f, 0.f, "Climb West");
		configParam(OUTPUT_MODE_PARAM, 0.f, 1.f, 0.f, "Output Mode");
		configParam(SELECT_SCALE_1_BUTTON_PARAM, 0.f, 1.f, 0.f, "Scale 1 Select");
		configParam(SCALE_1_ROOT_PARAM, 0.f, float(MAX_PITCH), float(DFLT_ROOT_NOTE), "Scale 1 Root");
		configParam(SELECT_SCALE_2_BUTTON_PARAM, 0.f, 1.f, 0.f, "Scale 2 Select");
		configParam(SCALE_2_ROOT_PARAM, 0.f, float(MAX_PITCH), float(DFLT_ROOT_NOTE), "Scale 2 Root");
		configParam(MUTE_NORTH_PARAM, 0.f, 1.f, 0.f, "Mute North");
		configParam(MUTE_SOUTH_PARAM, 0.f, 1.f, 0.f, "Mute South");
		configParam(MUTE_EAST_PARAM, 0.f, 1.f, 0.f, "Mute East");
		configParam(MUTE_WEST_PARAM, 0.f, 1.f, 0.f, "Mute West");
		configParam(SELECT_SCALE_3_BUTTON_PARAM, 0.f, 1.f, 0.f, "Scale 3 Select");
		configParam(SCALE_3_ROOT_PARAM, 0.f, float(MAX_PITCH), float(DFLT_ROOT_NOTE), "Scale 3 Root");
		configParam(SELECT_SCALE_4_BUTTON_PARAM, 0.f, 1.f, 0.f, "Scale 4 Select");
		configParam(SCALE_4_ROOT_PARAM, 0.f, float(MAX_PITCH), float(DFLT_ROOT_NOTE), "Scale 4 Root");
		configParam(ALT_SPEED_MODE_PARAM, 0.f, 1.f, 0.f, "Alternate Speed Mode");


		//leftExpander.producerMessage = leftMessages[0];
		//leftExpander.consumerMessage = leftMessages[1];

		//rightExpander.producerMessage = &rightMessages[0];
		//rightExpander.consumerMessage = &rightMessages[1];

		clock.setSampleRate(APP->engine->getSampleRate());
		clock.setBeatsPerMinute(DFLT_BPM);
		activeBpm = DFLT_BPM;

		roverStepClock.setStepSize( clock.getSamplesPerBeat() );
		paramUpdateClock.setStepSize( clock.getSampleRate() * 0.0005 ); // 0.5 millis 

		roverStepClock.sync();   // begin cycle now
		paramUpdateClock.sync(); // begin cycle now 
		roverStepClockEven = true;

		// done in initialize
		// activeSamplesPerBeat = clock.getSamplesPerBeat(); 
		initializeSampleCounters();

		activeRootNote = 36; // 'C'
		activeScaleIdx = 0;  // Major
		for (int k = 0; k < NUM_WALLS; k++) {
			walls[k]->setRootNote(activeRootNote); 
			// walls[k]->setScale(scaleTable[activeScaleIdx]);
		}

		setActiveScaleId(SCALE_1);
		setAlternateSpeedMode(SPEED_MODE_HALF);
		setOutputMode(OUTPUT_MODE_MINIMUM);
		setActivePolyphony(3);

		isPaused = false; 
		isReverse = false;
		isHerding = true;
		activeNoteLength = 1.f;

		redrawRequired = true;
		populateUiMatrix();
	}

	void onSampleRateChange() override {
		//DEBUG("onSampleRateChange()");
		clock.setSampleRate(APP->engine->getSampleRate());
		initializeSampleCounters();
	}

	void onReset() override {
		clearRoversTrigger.reset();
		clearObstaclesTrigger.reset();
		reverseTrigger.reset();
		runTrigger.reset();
		outputModeTrigger.reset();
		herdTrigger.reset();
		wallResetTrigger.reset();
		undoTrigger.reset();
		alternateSpeedModeTrigger.reset();
		for (int k = 0; k < NUM_SCALES; k++) {
			scaleTriggers[k].reset();
		}
		for (int k = 0; k < NUM_WALLS; k++){
			scaleDirectionTriggers[k].reset();
			wallMuteTriggers[k].reset();
		}
		clock.reset();
	}

	virtual json_t* dataToJson() override {
		json_t* rootJ = json_object();

		json_object_set_new(rootJ, "outputMode", json_integer(int(outputMode)));
		json_object_set_new(rootJ, "altSpeedMode", json_integer(int(alternateSpeedMode)));
		json_object_set_new(rootJ, "isPaused", json_integer(isPaused));
		json_object_set_new(rootJ, "isReverse", json_integer(isReverse));
		json_object_set_new(rootJ, "isHerding", json_integer(isHerding));
		json_object_set_new(rootJ, "polyphony", json_integer(activePolyphony));

		// TODO: consider making a struct for each wall as a placeholder for future variables 
		json_object_set_new(rootJ, "north_ascending", json_integer(northWall.isAscending()));
		json_object_set_new(rootJ, "east_ascending", json_integer(eastWall.isAscending()));
		json_object_set_new(rootJ, "south_ascending", json_integer(southWall.isAscending()));
		json_object_set_new(rootJ, "west_ascending", json_integer(westWall.isAscending()));

		json_object_set_new(rootJ, "north_muted", json_integer(northWall.isMuted()));
		json_object_set_new(rootJ, "east_muted", json_integer(eastWall.isMuted()));
		json_object_set_new(rootJ, "south_muted", json_integer(southWall.isMuted()));
		json_object_set_new(rootJ, "west_muted", json_integer(westWall.isMuted()));

		// scales 
		//  activeScaleId
		//  scale array 
		//    filepath
		//    template 
		//      name 
		//      pitches
		//      etc 

		json_t* json_scaleArray = json_array();
		for (int k = 0; k < NUM_SCALES; k++) {
			json_t* json_pitchesArray = json_array();
			for (int p = 0; p < scales[k].scaleTemplate.numPitches; p++) {
				json_array_append_new(json_pitchesArray, json_integer(scales[k].scaleTemplate.pitches[p]));
			}

			json_t* json_scaleTemplate = json_object();
			json_object_set_new(json_scaleTemplate, "name", json_string(scales[k].scaleTemplate.name.c_str()));
			json_object_set_new(json_scaleTemplate, "pitches", json_pitchesArray);

			json_t* json_scale = json_object();
			json_object_set_new(json_scale, "path", json_string(scales[k].filepath.c_str()));
			json_object_set_new(json_scale, "root", json_integer(scales[k].rootNote));
			json_object_set_new(json_scale, "template", json_scaleTemplate);

			json_array_append_new(json_scaleArray, json_scale);
		}
		json_object_set_new(rootJ, "activeScale", json_integer(activeScaleId));
		json_object_set_new(rootJ, "scales", json_scaleArray);

		json_t* json_roverArray = json_array();
		DoubleLinkListIterator<Rover> roverIter(activeRovers);
		while (roverIter.hasMore()) {
			Rover * pRover = roverIter.getNext();
			json_t* json_rover = json_object();
			json_object_set_new(json_rover, "row", json_integer(pRover->pos_row));
			json_object_set_new(json_rover, "col", json_integer(pRover->pos_col));
			json_object_set_new(json_rover, "bday", json_integer(pRover->birthday));
			json_object_set_new(json_rover, "type", json_integer(pRover->roverType)); // TODO: consider translate to string 'n', 'nw', etc 
			json_object_set_new(json_rover, "speed", json_integer(pRover->speed));    // TODO: consider translate to string 
			json_array_append_new(json_roverArray, json_rover);
			//DEBUG("Save Rover: row %d, col %d, type %d", pRover->pos_row, pRover->pos_col, pRover->roverType);
		}
		json_object_set_new(rootJ, "rovers", json_roverArray);

		json_t* json_obstacleArray = json_array();
		DoubleLinkListIterator<Obstacle> obstacleIter(activeObstacles);
		while (obstacleIter.hasMore()) {
			Obstacle * pObstacle = obstacleIter.getNext();
			json_t* json_obstacle = json_object();
			json_object_set_new(json_obstacle, "row", json_integer(pObstacle->pos_row));
			json_object_set_new(json_obstacle, "col", json_integer(pObstacle->pos_col));
			json_object_set_new(json_obstacle, "bday", json_integer(pObstacle->birthday));
			json_object_set_new(json_obstacle, "type", json_integer(pObstacle->obstacleType));
			// json_object_set_new(json_obstacle, "hitCount", json_integer(pObstacle->hitCount));
			json_array_append(json_obstacleArray, json_obstacle);
		}
		json_object_set_new(rootJ, "obstacles", json_obstacleArray);

		return rootJ;
	}

	json_int_t getJsonInteger(json_t* json_object, const char * key) const {
		json_t* json_string = json_object_get(json_object, key);
		if (json_string) {
			return json_integer_value(json_string);
		}
		return 0;
	}

	double getJsonReal(json_t* json_object, const char * key) const {
		json_t* json_string = json_object_get(json_object, key);
		if (json_string) {
			return json_real_value(json_string);
		}
		return 0.;
	}

	std::string getJsonString(json_t* json_object, const char * key) const {
		json_t* json_string = json_object_get(json_object, key);
		if (json_string) {
			return json_string_value(json_string);
		}
		return "";
	}

	virtual void dataFromJson(json_t* rootJ) override {
		json_t* jsonValue;
		size_t idx; 

		setOutputMode((OutputMode) getJsonInteger(rootJ, "outputMode"));
		setAlternateSpeedMode((SpeedMode) getJsonInteger(rootJ, "altSpeedMode"));
		setPaused(getJsonInteger(rootJ, "isPaused"));
		setReverse(getJsonInteger(rootJ, "isReverse"));
		setHerding(getJsonInteger(rootJ, "isHerding"));
		setActivePolyphony(getJsonInteger(rootJ, "polyphony"));

		setWallAscending(northWall, getJsonInteger(rootJ, "north_ascending"));
		setWallAscending(eastWall,  getJsonInteger(rootJ, "east_ascending"));
		setWallAscending(southWall, getJsonInteger(rootJ, "south_ascending"));
		setWallAscending(westWall,  getJsonInteger(rootJ, "west_ascending"));

		setWallMuted(northWall, getJsonInteger(rootJ, "north_muted"));
		setWallMuted(eastWall,  getJsonInteger(rootJ, "east_muted"));
		setWallMuted(southWall, getJsonInteger(rootJ, "south_muted"));
		setWallMuted(westWall,  getJsonInteger(rootJ, "west_muted"));

		// active scale id 
		// 
		json_t* json_scaleArray = json_object_get(rootJ, "scales");
		if (json_scaleArray) {
			json_array_foreach(json_scaleArray, idx, jsonValue) {
				ScaleId scaleId = (ScaleId) idx; 
				std::string path = getJsonString(jsonValue, "path"); 
				int rootNote = getJsonInteger(jsonValue, "root");
				json_t * json_scaleTemplate = json_object_get(jsonValue, "template");
				if (json_scaleTemplate) {
					std::string name = getJsonString(json_scaleTemplate, "name"); 
					json_t* json_pitchArray = json_object_get(json_scaleTemplate, "pitches");
					if (json_pitchArray) {
						size_t pitchIdx;
						json_t* jsonPitchValue;
						scales[scaleId].filepath = path;
						scales[scaleId].rootNote = rootNote;
						scales[scaleId].scaleTemplate.clear();
						scales[scaleId].scaleTemplate.name = name; 
						json_array_foreach(json_pitchArray, pitchIdx, jsonPitchValue) {
							int pitch = json_integer_value(jsonPitchValue);
							scales[scaleId].scaleTemplate.addPitch(pitch);
						}
					}
				}
			}
		}
		setActiveScaleId( (ScaleId) getJsonInteger(rootJ, "activeScale"));

		json_t* json_roverArray = json_object_get(rootJ, "rovers");
		if (json_roverArray) {
			json_array_foreach(json_roverArray, idx, jsonValue) {
				int row    = getJsonInteger(jsonValue, "row"); 
				int column = getJsonInteger(jsonValue, "col");
				float birthday = getJsonInteger(jsonValue, "bday");   // TODO: make constants for these strings
				int type   = getJsonInteger(jsonValue, "type");
				int speed  = getJsonInteger(jsonValue, "speed");
				// DEBUG("Load Rover: row %d, col %d, type %d, speed %d", row, column, type, speed);
				restoreRover((RoverType) type, row, column, birthday, (SpeedType) speed);
			}
		}

		json_t* json_obstacleArray = json_object_get(rootJ, "obstacles");
		if (json_obstacleArray) {
			json_array_foreach(json_obstacleArray, idx, jsonValue) {
				int row    = getJsonInteger(jsonValue, "row"); 
				int column = getJsonInteger(jsonValue, "col");
				float birthday = getJsonInteger(jsonValue, "bday");   // TODO: make constants for these strings
				int type   = getJsonInteger(jsonValue, "type");
				//int hitCount = 0;
				//json_string = json_object_get(jsonValue, "hitCount");
				//if (json_string) {
				//	hitCount = json_integer_value(json_string);
				//}
				restoreObstacle((ObstacleType) type, row, column, birthday);
			}
		}
	}

	void initializeSampleCounters() {

		//DEBUG("--- initializeSampleCounters: clock sample rate = %f", clock.getSampleRate());
		//DEBUG("--- initializeSampleCounters: clock bpm         = %f", clock.getBeatsPerMinute());
		//DEBUG("--- initializeSampleCounters: clock samples per beat = %f", clock.getSamplesPerBeat());

		activeSamplesPerBeat = clock.getSamplesPerBeat();

		roverStepClock.setStepSize( activeSamplesPerBeat ); 

		paramUpdateClock.setStepSize( clock.getSampleRate() * 0.0005 ); // 0.5 millis // TODO: const 

		if (clock.isConnectedToExternalClock()) {
			roverStepClock.sync();
			roverStepClockEven = true;
			paramUpdateClock.sync();
		}

		updateTimerLengths();

		//DEBUG("--- initializeSampleCounters: rover step size = %f", roverStepClock.stepSize);
		//DEBUG("--- initializeSampleCounters: param step size = %f", paramUpdateClock.stepSize);
	}

	void updateTimerLengths() {

		//DEBUG("-- Update Timer Lengths: stepsize %f, active gate length %f", roverStepClock.stepSize, activeNoteLength );

		float gateMultiplier = 1.f;

		if (activeNoteLength < 0.5) {
			// probably need pow() function here to spread the sizes out ??  
	        gateMultiplier = MIN_NOTE_LENGTH_BEATS + (activeNoteLength * 2.f);
		} else {
			gateMultiplier = 1.f + ((activeNoteLength - 0.5) * MAX_NOTE_LENGTH_BEATS);	
		} 

		wallStrikeHighlightSamples = roverStepClock.stepSize * gateMultiplier;
		collisionHighlightSamples = roverStepClock.stepSize * gateMultiplier;

//		wallStrikeHighlightSamples -= 1;  // TODO: EXPERIMENT to get rid of clicking 
//		collisionHighlightSamples -= 1;
	}

    // Convert [0,10] range to [0,1]
    // Return 0 if not connected 
    inline float getCvInput(int input_id) {
        if (inputs[input_id].isConnected()) {
            return (inputs[input_id].getVoltage() / 10.0);
        }
        return 0.f;
    }

	void process(const ProcessArgs& args) override {

//		bool rightExpanderPresent = (rightExpander.module && (rightExpander.module->model == modelTravelerExpander));
//		if (rightExpanderPresent) {
//			TravelerExpanderMessage * pMessage = static_cast<TravelerExpanderMessage*>(rightExpander.consumerMessage);
//			DEBUG("Counter = %d", pMessage->counter);
//		}


		//redrawRequired = false;
		//voiceUpdateRequired = false;

		clock.tick(inputs[CLOCK_CV_INPUT]);

		if (fabs(clock.getSamplesPerBeat() - activeSamplesPerBeat) >= 20.f) {
			//DEBUG(" process - after clock.tick() -- RATE CHANGE DETECTED");
			//DEBUG("         - clock.samples per beat = %f", clock.getSamplesPerBeat());
			//DEBUG("         - rover clock step size  = %f", roverStepClock.stepSize);
			initializeSampleCounters();
		}


		roverStepClock.tick();
		
		paramUpdateClock.tick();

		if (runTrigger.process(params[RUN_BUTTON_PARAM].getValue() + inputs[RUN_CV_INPUT].getVoltage())) {
			isPaused = !isPaused;
			redrawRequired = true;
		}
		lights[RUN_LED_LIGHT].setBrightness( (!isPaused) ); 

		if (reverseTrigger.process(params[REVERSE_BUTTON_PARAM].getValue() + inputs[REVERSE_CV_INPUT].getVoltage())) {
			isReverse = !isReverse;
			reverseAllRovers();
			redrawRequired = true;
		}		
		lights[REVERSE_LED_LIGHT].setBrightness(isReverse);

		if (outputModeTrigger.process(params[OUTPUT_MODE_PARAM].getValue())) {
			selectNextOutputMode();
		}

		if (herdTrigger.process(params[HERD_BUTTON_PARAM].getValue())) {
			isHerding = !isHerding;
		}
		lights[HERD_LED_LIGHT].setBrightness( isHerding ); 

		if (paramUpdateClock.isLeadingEdge()) {
			processSpeed();
			processScaleSelect();
			processRootNote();
			processWalls();
			processClearAction();
			processQueuedObstacleActions();
			processQueuedRoverActions();
			processResizing();
			processProbabilityThresholds();
			processNoteLength();
		}


		if (roverStepClock.isLeadingEdge()) {
			processMovement(SPEED_NORMAL);
			if (alternateSpeedMode == SPEED_MODE_HALF)  {
				if (roverStepClockEven) {
					processMovement(SPEED_ALT);
				}
			} else if (alternateSpeedMode == SPEED_MODE_DOUBLE)  {
				processMovement(SPEED_ALT);				
			}

			roverStepClockEven = (!roverStepClockEven);
		}

 		if (roverStepClock.isTrailingEdge()) {
			if (alternateSpeedMode == SPEED_MODE_DOUBLE)  {
				processMovement(SPEED_ALT);				
			}
		}

		processGateClosures();

		if (voiceUpdateRequired) {
			addOutputVoices();
			voiceUpdateRequired = false;
		}

		produceOutput();

		if (redrawRequired) {
			markUiDirty();
			redrawRequired = false;
		}

		if (uiUpdatePending) {
			pushUiUpdates();
		}
	}

	void processSpeed() { 

		if (! clock.isConnectedToExternalClock()) {
			float bpmValue = params[RUN_RATE_PARAM].getValue();

			// bpm range 15=(15*1) ... 960=(15*64))
			//   15*2^^n where n is 0..6 

			bpmValue = MIN_BPM * pow(2, bpmValue * 6.0);
			if (bpmValue != activeBpm) {
				activeBpm = bpmValue;
				clock.setBeatsPerMinute(activeBpm);
				initializeSampleCounters();
			}
		}

		if (alternateSpeedModeTrigger.process(params[ALT_SPEED_MODE_PARAM].getValue())) {
			selectNextAlternateSpeedMode();
		}
	}

	void processScaleSelect() {

		if (inputs[SCALE_SELECTOR_CV_INPUT].isConnected()) {
			float scale_cv = inputs[SCALE_SELECTOR_CV_INPUT].getVoltage();
			int scaleId = clamp(scale_cv * 0.1 * float(NUM_SCALES), 0, NUM_SCALES - 1); // unipolar 
			if (scaleId != activeScaleId) {
				setActiveScaleId((ScaleId)scaleId);
				redrawRequired = true;
			}
		} else { 
			for (int k = 0; k < NUM_SCALES; k++) {
				if (scaleTriggers[k].process(params[ scaleParams[k].scaleSelectParamId ].getValue())) {
					setActiveScaleId( scaleParams[k].scaleId );
					redrawRequired = true;
				}
			}
		}
	}

	void processRootNote() {

		for (int k = 0; k < NUM_SCALES; k++) {
			int paramId = scaleParams[k].rootNoteParamId;
			ScaleId scaleId = scaleParams[k].scaleId;

			float rootNoteValue = params[paramId].getValue(); 
			int rootNote = clamp(rootNoteValue, 0.f, float(MAX_PITCH)); // 0..120
			if (rootNote != scales[scaleId].rootNote) {
				setRootNote(scaleId, rootNote);
				redrawRequired = true;
			}
		}
	}

	void applyRootNote(int rootNote) { // 0..11, C..B
		activeRootNote = rootNote;
		for (int k = 0; k < NUM_WALLS; k++) {
			walls[k]->setRootNote(activeRootNote);
		}
	}

	float getValuePlusMinusOne(Param & param) {
		return params[param.paramId].getValue(); // assume configured for -1..0..1 range 
	}

	float getValuePlusMinusOne(ParamWithCv & paramWithCv) {
		if (inputs[paramWithCv.cvId].isConnected()) {
			return (inputs[paramWithCv.cvId].getVoltage() - 5.f) / 5.f;
		}
		return params[paramWithCv.paramId].getValue(); // assume configured for -1..0..1 range 
	}

	void processWalls() {

		bool resetWalls = false; 
		if (wallResetTrigger.process(params[RESET_WALLS_PARAM].getValue())) {
			resetWalls = true;
			//DEBUG("---- RESET Walls");
		}

		for (int k = 0; k < NUM_WALLS; k++) {
			int shift  = getValuePlusMinusOne(wallParamsTable[k].shift)  * float((ROVER_FIELD_DIMENSION - 1));
			int rotate = getValuePlusMinusOne(wallParamsTable[k].rotate) * float((ROVER_FIELD_DIMENSION - 1));
			int climb = getValuePlusMinusOne(wallParamsTable[k].climb)* float((ROVER_FIELD_DIMENSION - 1));

			shift  = clamp(shift,  -(ROVER_FIELD_DIMENSION - 1), (ROVER_FIELD_DIMENSION - 1));
			rotate = clamp(rotate, -(ROVER_FIELD_DIMENSION - 1), (ROVER_FIELD_DIMENSION - 1));
			climb  = clamp(climb,  -(ROVER_FIELD_DIMENSION - 1), (ROVER_FIELD_DIMENSION - 1));
			
			shift = -shift; // reverse value so that knob movement left-to-right matches scale movement
			rotate = -rotate; 
			
			// TODO: use dual colored light, one color for ascending, another for descending .. need 2 lights to do this 
			if (scaleDirectionTriggers[ k ].process(params[ wallParamsTable[k].ascending.paramId ].getValue())) {
				walls[k]->setAscending( ! walls[k]->isAscending() );
				redrawRequired = true;
			}
			lights[ wallParamsTable[k].ascending.lightId ].setBrightness( (!walls[k]->isAscending()) ); 

			if (wallMuteTriggers[ k ].process(params[ wallParamsTable[k].mute.paramId ].getValue())) {
				walls[k]->setMuted( ! walls[k]->isMuted() );
				redrawRequired = true;
			}
			lights[ wallParamsTable[k].mute.lightId ].setBrightness( (walls[k]->isMuted()) ); 

			if (walls[k]->getRotation() != shift) { 
				walls[k]->setRotation(shift);
				redrawRequired = true;
			}

			if (walls[k]->getAutoRotate() != rotate) {
				walls[k]->setAutoRotate(rotate);
			}

			if (walls[k]->getClimb() != climb) {
				walls[k]->setClimb(climb);
			}

			if (resetWalls) {
				walls[k]->resetRotation();
				redrawRequired = true;
			}
		}
	}

	void processClearAction() {
		if (clearRoversTrigger.process(params[CLEAR_ROVERS_PARAM].getValue())) {
			if (activeRovers.size() > 0) {
				clearRovers();
				redrawRequired = true;
			}
		}
		
		if (clearObstaclesTrigger.process(params[CLEAR_OBSTACLES_PARAM].getValue())) {
			if (activeObstacles.size() > 0) {
				clearObstacles();
				redrawRequired = true;
			}
		}
	
		if (undoTrigger.process(params[UNDO_PARAM].getValue())) {
			removeNewestOccupant();
			redrawRequired = true;
		}
	}

	void clearRovers() {
		matrix.clearRovers();
		Rover * pRover;
		while ((pRover = activeRovers.popFront())) {
			roverPool.pushTail(pRover);
		}
	}

	void clearObstacles() {
		matrix.clearObstacles();
		Obstacle * pObstacle;
		while ((pObstacle = activeObstacles.popFront())) {
			obstaclePool.pushTail(pObstacle);
		}
	}

	void removeNewestOccupant() { 
		Rover * pNewestRover = lookupNewestRover();
		Obstacle * pNewestObstacle = lookupNewestObstacle();

		if (pNewestRover && pNewestObstacle) {
			if (pNewestRover->birthday > pNewestObstacle->birthday) {
				removeRover(pNewestRover);
			} else { 
				removeObstacle(pNewestObstacle);
			}
		} else if (pNewestRover) {
			removeRover(pNewestRover);
		} else if (pNewestObstacle) {
			removeObstacle(pNewestObstacle);
		}
	}

	// May return NULL 	
	Rover * lookupNewestRover() {
		Rover * pNewestRover = activeRovers.peekFront(); 
		DoubleLinkListIterator<Rover> iter(activeRovers);
		while (iter.hasMore()) {
			Rover * pRover = iter.getNext();
			if (pRover->birthday > pNewestRover->birthday) {
				pNewestRover = pRover;
			}
		} 
		return pNewestRover;
	}
	
	// May return NULL 	
	Obstacle * lookupNewestObstacle() {
		Obstacle * pNewestObstacle = activeObstacles.peekFront();
		DoubleLinkListIterator<Obstacle> iter(activeObstacles);
		while (iter.hasMore()) {
			Obstacle * pObstacle = iter.getNext();
			if (pObstacle->birthday > pNewestObstacle->birthday) {
				pNewestObstacle = pObstacle;
			}
		} 
		return pNewestObstacle;
	}

	void processQueuedObstacleActions() {
		ObstacleAction * pObstacleAction;
		while((pObstacleAction = obstacleActionQueue.dequeue())) {

			int row = pObstacleAction->row;
			int column = pObstacleAction->column;

			if (matrix.dimensions.isOutside(row, column)) {
				// DEBUG("Obstacle Action ADD: row %d, cell %d, type %d -- IGNORE -- position is outside walls", row, column, pObstacleAction->obstacleType);
			} else if (pObstacleAction->actionType == OBSTACLE_ACTION_ADD) {
				addObstacle(pObstacleAction->obstacleType, row, column);
				redrawRequired = true;
			} else if (pObstacleAction->actionType == OBSTACLE_ACTION_REMOVE) {
				removeObstaclesAt(row, column);
				redrawRequired = true;
			}
			retireObstacleAction(pObstacleAction);
		}
	}

	void addObstacle(ObstacleType obstacleType, int row, int column) { 
		Occupant * pOccupant = matrix.getOccupant(row, column);
		if (pOccupant) {
			if (pOccupant->isRover) {
				// do nothing
				// Always accept obstacles, replace cell if occupied 
				// will temporarily cover a rover but it should pop out in the next forward cycle
				// TODO: is the matrix[][] tracking rovers ?? i think that became OBE
			} else {
				// occupant must be Obstacle // TODO: add explicit check for this ?
				Obstacle * pObstacle = static_cast<Obstacle*>(pOccupant);
				activeObstacles.remove(pObstacle);
				retireObstacle(pObstacle);
				matrix.setOccupant(NULL, row, column);
			}
		}

		Obstacle * pObstacle = createObstacle(obstacleType, row, column);
		matrix.setOccupant(pObstacle, row, column);
		activeObstacles.pushTail(pObstacle);
	}

	void removeObstacle(Obstacle * pObstacle) {
		if (pObstacle) {
			matrix.setOccupant(NULL, pObstacle->pos_row, pObstacle->pos_col);
			activeObstacles.remove(pObstacle);
			retireObstacle(pObstacle);
		}
	}

	void restoreObstacle(ObstacleType obstacleType, int row, int column, int birthday) {
		Obstacle * pObstacle = allocateObstacle();
		pObstacle->initialize(obstacleType, row, column, birthday, matrix.dimensions);
		activeObstacles.pushTail(pObstacle);
		matrix.setOccupant(pObstacle, row, column);
	}

	void removeObstaclesAt(int row, int column) {
		PtrArray<Obstacle> retired; 
		
		DoubleLinkListIterator<Obstacle> iter(activeObstacles); 
		while (iter.hasMore()) {
			Obstacle * pObstacle = iter.getNext();
			if (pObstacle->getRow() == row && pObstacle->getCol() == column) {
				retired.append(pObstacle);
			}
		}

		for (int k = 0; k < retired.getNumMembers(); k++) {
			removeObstacle(retired.getAt(k));
		}
	}

	void processQueuedRoverActions() {
		RoverAction * pRoverAction;
		while((pRoverAction = roverActionQueue.dequeue())) {
			int row = pRoverAction->row;
			int column = pRoverAction->column;
			if (matrix.dimensions.isOutside(row, column)) {
				// DEBUG("Rover Action ADD: row %d, cell %d, type %d -- IGNORE -- position is outside walls", row, column, pRoverAction->roverType);			
			} else if (pRoverAction->actionType == ROVER_ACTION_ADD) {
				if (matrix.isUnoccupied(row, column)) {	// Disregard action if cell is occupied 
					addRover(pRoverAction->roverType, row, column);
					redrawRequired = true;
				}
			}
			else if (pRoverAction->actionType == ROVER_ACTION_REMOVE) {
				removeRoversAt(row,column);
				redrawRequired = true;
			}
			retireRoverAction(pRoverAction);
		}
	}

	void addRover(RoverType roverType, int row, int column) {
		Rover * pRover = createRover(roverType, row, column);
		activeRovers.pushTail(pRover);
	}

	void removeRover(Rover * pRover) {
		if (pRover) {
			activeRovers.remove(pRover);
			retireRover(pRover);
		}
	}

	void restoreRover(RoverType roverType, int row, int column, int birthday, SpeedType speed) {
		Rover * pRover = allocateRover();
		pRover->initialize(roverType, row, column, birthday, matrix.dimensions);
		pRover->speed = speed;
		activeRovers.pushTail(pRover);
	}

	void removeRoversAt(int row, int column) {
		PtrArray<Rover> retired; 
		
		DoubleLinkListIterator<Rover> iter(activeRovers); 
		while (iter.hasMore()) {
			Rover * pRover = iter.getNext();
			if (pRover->getRow() == row && pRover->getCol() == column) {
				retired.append(pRover);
			}
		}

		for (int k = 0; k < retired.getNumMembers(); k++) {
			removeRover(retired.getAt(k));
		}
	}

	void processResizing() {
		float numRowsFloat = params[ROW_COUNT_PARAM].getValue();
		float numColumnsFloat = params[COLUMN_COUNT_PARAM].getValue();

		numRowsFloat += params[ROW_COUNT_ATTENUVERTER_PARAM].getValue() * getCvInput(ROW_COUNT_CV_INPUT);
		numColumnsFloat += params[COLUMN_COUNT_ATTENUVERTER_PARAM].getValue() * getCvInput(COLUMN_COUNT_CV_INPUT);

		numRowsFloat = clamp(numRowsFloat, 0.f, 1.0f);
		numColumnsFloat = clamp(numColumnsFloat, 0.f, 1.0f);

		// DEBUG("Size Check: param rows %f, param cols %f", numRows, numColumns);

		int numRows    = (numRowsFloat    * (ROVER_FIELD_DIMENSION - 1));
		int numColumns = (numColumnsFloat * (ROVER_FIELD_DIMENSION - 1));

		numRows    = clamp(numRows + 1,    1, ROVER_FIELD_ROWS);
		numColumns = clamp(numColumns + 1, 1, ROVER_FIELD_COLUMNS);

		if (numRows != matrix.getNumRows() || numColumns != matrix.getNumColumns()) {

			int row_delta = numRows - matrix.getNumRows();
			int column_delta = numColumns - matrix.getNumColumns();
			
			//DEBUG("Size Change: was rows %d , cols %d", matrix.getNumRows(), matrix.getNumColumns());
			//DEBUG("              is rows %d , cols %d", numRows, numColumns);

			matrix.dimensions.setRowsCols(numRows, numColumns);

			northWall.numVisibleBlocks = numColumns;
			eastWall.numVisibleBlocks = numRows;
			southWall.numVisibleBlocks = numColumns;
			westWall.numVisibleBlocks = numRows;

			if (isHerding) {	
				DoubleLinkListIterator<Rover> iter(activeRovers);
				while (iter.hasMore()) {
					Rover * pRover = iter.getNext();
					//if (pRover->isOutsideWalls()) {
					//	DEBUG("Rover at %d, %d is outside the walls", pRover->pos_row, pRover->pos_col);
					//}
					if (pRover->pos_row >= numRows) {
						pRover->pos_row += row_delta;
					}
					
					if (pRover->pos_col >= numColumns) {
						pRover->pos_col += column_delta;
					}
				}
			}

			//DEBUG("SIZE CHANGE occurred: redraw..");
			redrawRequired = true;

		}
	}

	void processProbabilityThresholds() { 

		obstacleProbability = params[OBSTACLE_PROBABILITY_PARAM].getValue(); 
		obstacleProbability += params[OBSTACLE_PROBABILITY_ATTENUVERTER_PARAM].getValue() * getCvInput(OBSTACLE_PROBABILITY_CV_INPUT);
		obstacleProbability = clamp(obstacleProbability, 0.f, 1.f);

		wallProbability = params[WALL_PROBABILITY_PARAM].getValue(); 
		wallProbability += params[WALL_PROBABILITY_ATTENUVERTER_PARAM].getValue() * getCvInput(WALL_PROBABILITY_CV_INPUT);
		wallProbability = clamp(wallProbability, 0.f, 1.f);

		noteProbability = params[NOTE_PROBABILITY_PARAM].getValue(); 
		noteProbability += params[NOTE_PROBABILITY_ATTENUVERTER_PARAM].getValue() * getCvInput(NOTE_PROBABILITY_CV_INPUT);
		noteProbability = clamp(noteProbability, 0.f, 1.f);

		steadyProbability = params[STEADY_PROBABILITY_PARAM].getValue(); 
		steadyProbability += params[STEADY_PROBABILITY_ATTENUVERTER_PARAM].getValue() * getCvInput(STEADY_PROBABILITY_CV_INPUT);
		steadyProbability = clamp(steadyProbability, 0.f, 1.f);
	}

	void processNoteLength() { 

		float noteLength; 

		noteLength = params[NOTE_LENGTH_PARAM].getValue(); 
		noteLength += params[NOTE_LENGTH_ATTENUVERTER_PARAM].getValue() * getCvInput(NOTE_LENGTH_CV_INPUT);
		noteLength = clamp(noteLength, 0.f, 1.f);
		if (noteLength != activeNoteLength) {
			//DEBUG("processNoteLength: gateValue %f, activeNoteLength %f", gateValue, activeNoteLength);
			activeNoteLength = noteLength;
			updateTimerLengths();
		}
	}



	void processMovement(SpeedType speedType) {

		// NOTE: consider doing some of this prep type work in a midpoint before the step begins
		// can spread out some of the prep and cleanup that way -- be careful though that
		// the UI has had time to update before clearing critical data structures 
		northWall.applyAutoRotation();
		eastWall.applyAutoRotation();
		southWall.applyAutoRotation();
		westWall.applyAutoRotation();

		if (isPaused) {
			return; 
		}

		// DEBUG("ROVER MOVEMENT occurred: redraw..");
		redrawRequired = true;

		resolveCollisions(speedType);
		repositionDisplacedRovers();
		moveRovers(speedType);
		repositionDisplacedRovers();
		recordCollisions();
	}

	void moveRovers(SpeedType speedType) {

		//DEBUG("Move Rovers: --- size %d, %d -- num rovers %d", matrix.dimensions.num_rows, matrix.dimensions.num_cols, activeRovers.size());
		DoubleLinkListIterator<Rover> iter(activeRovers);
		while (iter.hasMore()) {
			Rover * pRover = iter.getNext();

			if (pRover->getSpeed() != speedType) {
				continue;
			}

			// defensive coding - force rovers that somehow went stray back into the matrix 
			pRover->clamp();

			if (pRover->isOutsideWalls()) {
				//DEBUG("Move: rover %d at %d, %d is OUTSIDE", pRover->roverType, pRover->pos_row, pRover->pos_col);
				continue; // if rover is outside the active area, freeze position 
			}

			if (random.generateZeroToOne() > steadyProbability) {
				if (random.generateZeroToOne() > 0.5) {
					pRover->moveLateralCw();
				} else { 
					pRover->moveLateralCcw();
				}
			}

			//DEBUG("Move: rover %d at %d, %d", pRover->roverType, pRover->pos_row, pRover->pos_col);

			bool doAdvance = true;

			switch (pRover->roverType) {
				case ROVER_NORTH:
					if (pRover->isAtNorthWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							pRover->moveToSouthRow();   // pass through 
//							doAdvance = false;
						} else {
							processWallStrikeHorizontal(northWall, pRover);
							pRover->reflectNorth();
						}
					}
					break;
				case ROVER_NORTHEAST:
					if (pRover->isAtNorthEastCorner()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to south west corner
							// pRover->moveToSouthRow();
							pRover->moveToSouthWestCorner();
							doAdvance = false;
						} else {
							processWallStrikeCorner(northWall, eastWall, pRover);
							pRover->reverse();
						}
					} else if (pRover->isAtNorthWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to south wwall
							pRover->moveToSouthRow();
							doAdvance = false;
						} else {
							processWallStrikeHorizontal(northWall, pRover);
							pRover->reflectNorth();
						}
					} else if (pRover->isAtEastWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to south wwall
							pRover->moveToWestColumn();
							doAdvance = false;
						} else {
							processWallStrikeVertical(eastWall, pRover);
							pRover->reflectEast();
						}
					}
					break;
				case ROVER_EAST:
					if (pRover->isAtEastWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							pRover->moveToWestColumn();   // pass through 
							doAdvance = false;
						} else {
							processWallStrikeVertical(eastWall, pRover);
							pRover->reflectEast();
						}
					}
					break;
				case ROVER_SOUTHEAST:
					if (pRover->isAtSouthEastCorner()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to south west corner
							pRover->moveToNorthWestCorner();
							doAdvance = false;
						} else {
							processWallStrikeCorner(southWall, eastWall, pRover);
							pRover->reverse();
						}
					} else if (pRover->isAtSouthWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to opposite wwall
							pRover->moveToNorthRow();
							doAdvance = false;
						} else {
							processWallStrikeHorizontal(southWall, pRover);
							pRover->reflectSouth();
						}
					} else if (pRover->isAtEastWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to south wwall
							pRover->moveToWestColumn();
							doAdvance = false;
						} else {
							processWallStrikeVertical(eastWall, pRover);
							pRover->reflectEast();
						}
					}
					break;
				case ROVER_SOUTH:
					if (pRover->isAtSouthWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							pRover->moveToNorthRow();   // pass through 
							doAdvance = false;
						} else {
							processWallStrikeHorizontal(southWall, pRover);
							pRover->reflectSouth();
						}
					}
					break;
				case ROVER_SOUTHWEST:
					if (pRover->isAtSouthWestCorner()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to opposite corner
							pRover->moveToNorthEastCorner();
							doAdvance = false;
						} else {
							processWallStrikeCorner(southWall, westWall, pRover);
							pRover->reverse();
						}
					} else if (pRover->isAtSouthWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to opposite wwall
							pRover->moveToNorthRow();
							doAdvance = false;
						} else {
							processWallStrikeHorizontal(southWall, pRover);
							pRover->reflectSouth();
						}
					} else if (pRover->isAtWestWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to south wwall
							pRover->moveToEastColumn();
							doAdvance = false;
						} else {
							processWallStrikeVertical(westWall, pRover);
							pRover->reflectWest();
						}
					}
					break;
				case ROVER_WEST:
					if (pRover->isAtWestWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							pRover->moveToEastColumn();   // pass through 
							doAdvance = false;
						} else {
							processWallStrikeVertical(westWall, pRover);
							pRover->reflectWest();
						}
					}
					break;
				case ROVER_NORTHWEST:
					if (pRover->isAtNorthWestCorner()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to opposite corner
							pRover->moveToSouthEastCorner();
							doAdvance = false;
						} else {
							processWallStrikeCorner(northWall, westWall, pRover);
							pRover->reverse();
						}
					} else if (pRover->isAtNorthWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to opposite wall
							pRover->moveToSouthRow();
							doAdvance = false;
						} else {
							processWallStrikeHorizontal(northWall, pRover);
							pRover->reflectNorth();
						}
					} else if (pRover->isAtWestWall()) {
						doAdvance = false;
						if (random.generateZeroToOne() > wallProbability) {
							// pass through to south wwall
							pRover->moveToEastColumn();
							doAdvance = false;
						} else {
							processWallStrikeVertical(westWall, pRover);
							pRover->reflectWest();
						}
					}
					break;
				default:
					break;
			}

			int row_before = pRover->pos_row;
			int col_before = pRover->pos_col;

			if (doAdvance) {
				pRover->advance();
				//DEBUG(" - after advance rover type %d at %d, %d", pRover->roverType, pRover->pos_row, pRover->pos_col);
			}

			if (matrix.isObstacle(pRover->pos_row, pRover->pos_col)) {
				applyObstacle(pRover);
			}

			pRover->setStuck( pRover->pos_row == row_before && pRover->pos_col == col_before );
			// DEBUG(" - Rover type %d at %d, %d, stuck count %d", pRover->roverType, pRover->pos_row, pRover->pos_col, pRover->stuckCount);
		
		}
	}

	void processWallStrikeHorizontal(Wall & wall, Rover * pRover) {
		if (pRover->collisionCount == 0) {
			processWallStrike(wall, pRover->pos_col);
		}
		pRover->moveHorizontal(wall.getClimb());
	}

	void processWallStrikeVertical(Wall & wall, Rover * pRover) {
		if (pRover->collisionCount == 0) {
			processWallStrike(wall, pRover->pos_row);
		}
		pRover->moveVertical(wall.getClimb());
	}

	void processWallStrikeCorner(Wall & horizontalWall, Wall & verticalWall, Rover * pRover) {
		if (pRover->collisionCount == 0) {
			processWallStrike(horizontalWall, pRover->pos_col);
			processWallStrike(verticalWall, pRover->pos_row);
		}
	}


	void processWallStrike(Wall & wall, int block_idx) {
		if (random.generateZeroToOne() > noteProbability) {
			return;
		}

		if (! wall.isMuted()) {
			// record pitch active
			// perform auto rotation if enabled 
			int pitch_interval_idx = wall.strike(block_idx);

			// record delayed record to shut gate and disable pitch in future 
			addWallStrikeTimer(wall.direction, pitch_interval_idx, wallStrikeHighlightSamples);

			voiceUpdateRequired = true; 
		}
	}

// DEBUG 
	void debug_printWallStrikeTimerList(std::string hint) {
		DEBUG("WallStrikeTimerList: %s:  %d members, pHead %p, pTail %p", 
			hint.c_str(),
			wallStrikeTimerList.numMembers, 
			wallStrikeTimerList.pHead, 
			wallStrikeTimerList.pTail);
		int memberCount = 0; 
		WallStrikeTimer * pTimer = wallStrikeTimerList.pHead;
		while (pTimer) {
			DEBUG(" Timer[%d] %p: wall %d, pitch %d, delay %ld, pPrev %p, pNext %p", 
				memberCount, 
				pTimer, 
				pTimer->wallDirection, pTimer->pitchInterval_idx, pTimer->delay, 
				pTimer->pPrev, pTimer->pNext);
			memberCount++; 

			pTimer = (WallStrikeTimer*) pTimer->pNext;
		}
	}
// end DEBUG 

	void applyObstacle(Rover * pRover) {
		Obstacle * pObstacle = matrix.getObstacle(pRover->pos_row, pRover->pos_col);
		if (! pObstacle) {
			return;
		}

		if (random.generateZeroToOne() > obstacleProbability) {
			return;
		}

		switch(pObstacle->obstacleType) {
			case OBSTACLE_MIRROR_HORIZONTAL:
				if (pRover->isHorizontal()) {
					pRover->reverse();
					pRover->advance();
				}
				break;
			case OBSTACLE_MIRROR_HORIZONTAL_FLIP:
				if (pRover->isHorizontal()) {
					pRover->reverse();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_VERTICAL_FLIP;
				}
				break;
			case OBSTACLE_MIRROR_VERTICAL:
				if (pRover->isVertical()) {
					pRover->reverse();
					pRover->advance();
				}
				break;
			case OBSTACLE_MIRROR_VERTICAL_FLIP:
				if (pRover->isVertical()) {
					pRover->reverse();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_HORIZONTAL_FLIP;
				}
				break;
			case OBSTACLE_MIRROR_SQUARE:
				if (pRover->isHorizontal() || pRover->isVertical()) {
					pRover->reverse();
				}
				else {// diagonal 
					if (pObstacle->selectAlternating()) {
						pRover->rotate45Cw();
					} else {
						pRover->rotate45Ccw();
					}
				}
				pRover->advance();
				break;
			case OBSTACLE_MIRROR_DIAMOND:
				if (pRover->isDiagonal()) {
					pRover->reverse();
				}
				else {// horizontal or vertical 
					if (pObstacle->selectAlternating()) {
						pRover->rotate45Cw();
					} else {
						pRover->rotate45Ccw();
					}
				}
				pRover->advance();
				break;
			case OBSTACLE_MIRROR_NE_SW:
				if (pRover->isNorthEast() || pRover->isSouthWest()) {
					pRover->reverse();
					pRover->advance();
				} else if (pRover->isNorth()) {
					pRover->goWest();
					pRover->advance();
				} else if (pRover->isEast()) {
					pRover->goSouth();
					pRover->advance();
				} else if (pRover->isSouth()) {
					pRover->goEast();
					pRover->advance();
				} else if (pRover->isWest()) {
					pRover->goNorth();
					pRover->advance();
				}
				break;
			case OBSTACLE_MIRROR_NE_SW_FLIP:
				if (pRover->isNorthEast() || pRover->isSouthWest()) {
					pRover->reverse();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NW_SE_FLIP;
				} else if (pRover->isNorth()) {
					pRover->goWest();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NW_SE_FLIP;
				} else if (pRover->isEast()) {
					pRover->goSouth();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NW_SE_FLIP;
				} else if (pRover->isSouth()) {
					pRover->goEast();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NW_SE_FLIP;
				} else if (pRover->isWest()) {
					pRover->goNorth();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NW_SE_FLIP;
				}
				break;
			case OBSTACLE_MIRROR_NW_SE:
				if (pRover->isNorthWest() || pRover->isSouthEast()) {
					pRover->reverse();
				} else if (pRover->isNorth()) {
					pRover->goEast();
					pRover->advance();
				} else if (pRover->isEast()) {
					pRover->goNorth();
					pRover->advance();
				} else if (pRover->isSouth()) {
					pRover->goWest();
					pRover->advance();
				} else if (pRover->isWest()) {
					pRover->goSouth();
					pRover->advance();
				}
				break;
			case OBSTACLE_MIRROR_NW_SE_FLIP:
				if (pRover->isNorthWest() || pRover->isSouthEast()) {
					pRover->reverse();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NE_SW_FLIP;
				} else if (pRover->isNorth()) {
					pRover->goEast();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NE_SW_FLIP;
				} else if (pRover->isEast()) {
					pRover->goNorth();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NE_SW_FLIP;
				} else if (pRover->isSouth()) {
					pRover->goWest();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NE_SW_FLIP;
				} else if (pRover->isWest()) {
					pRover->goSouth();
					pRover->advance();
					pObstacle->obstacleType = OBSTACLE_MIRROR_NE_SW_FLIP;
				}
				break;
			case OBSTACLE_WEDGE_NORTH:
				if (pRover->isNorth()) {
					if (pObstacle->selectAlternating()) {
						pRover->goEast();
					} else {
						pRover->goWest();
					}
					pRover->advance();
				}
				else if (pRover->isEast() || pRover->isWest()) {
					pRover->goSouth();
					pRover->advance();
				}
				break;
			case OBSTACLE_WEDGE_EAST:
				if (pRover->isEast()) {
					if (pObstacle->selectAlternating()) {
						pRover->goNorth();
					} else {
						pRover->goSouth();
					}
					pRover->advance();
				}
				else if (pRover->isNorth() || pRover->isSouth()) {
					pRover->goWest();
					pRover->advance();
				}
				break;
			case OBSTACLE_WEDGE_SOUTH:
				if (pRover->isSouth()) {
					if (pObstacle->selectAlternating()) {
						pRover->goEast();
					} else {
						pRover->goWest();
					}
					pRover->advance();
				}
				else if (pRover->isEast() || pRover->isWest()) {
					pRover->goNorth();
					pRover->advance();
				}
				break;
			case OBSTACLE_WEDGE_WEST:
				if (pRover->isWest()) {
					if (pObstacle->selectAlternating()) {
						pRover->goNorth();
					} else {
						pRover->goSouth();
					}
					pRover->advance();
				}
				else if (pRover->isNorth() || pRover->isSouth()) {
					pRover->goEast();
					pRover->advance();
				}
				break;
			case OBSTACLE_DETOUR:
				// Go clockwise or counterclockwise to get around the obstacle
				if (pObstacle->selectAlternating()) {
					pRover->moveLateralCw();
				} else {
					pRover->moveLateralCcw();
				}
				pRover->advance();
				break;
			case OBSTACLE_WORMHOLE:
				pRover->setType( selectRandomDirection() );
				displacedRovers.append(pRover);
				break;
			case OBSTACLE_SPIN:
				pRover->setType( selectRandomDirection() );
				pRover->advance();
				break;
			case OBSTACLE_SPEED_NORMAL:
				pRover->setSpeed( SPEED_NORMAL );
				break;
			case OBSTACLE_SPEED_ALT:
				pRover->setSpeed( SPEED_ALT );
				break;
			case OBSTACLE_SPEED_TOGGLE:
				pRover->toggleSpeed();
				break;
			default:
				DEBUG(" - UNHANDLED obstacle type %d at %d, %d", pObstacle->obstacleType, pObstacle->pos_row, pObstacle->pos_col);
				break;
		}
	}

	RoverType selectRandomDirection() { 
		int direction = int(random.generateZeroToOne() * float(NUM_ROVER_TYPES - 1));
		return (RoverType) clamp(direction, 0, NUM_ROVER_TYPES -1); 
	}

	void recordCollisions() {

		collisionTracker.clear();

		//DEBUG("RECORD Collisions: num rovers %d", activeRovers.numMembers);

		DoubleLinkListIterator<Rover> iter(activeRovers);
		while (iter.hasMore()) {
			Rover * pRover = iter.getNext();
 			collisionTracker.recordRoverPosition(pRover);
		}

		//DEBUG("- Collision: found %d collisions", collisionTracker.getNumCollisions());
		iter.reset();
		while (iter.hasMore()) {
			Rover * pRover = iter.getNext();
			pRover->setColliding( collisionTracker.isCollidedRover(pRover) );
			// DEBUG("- Collision: rover %d at %d, %d collision count = %d", pRover->roverType, pRover->pos_row, pRover->pos_col, pRover->collisionCount);
		}
	}

	void resolveCollisions(SpeedType speedType) {

		DoubleLinkListIterator<Rover> iter(activeRovers);
		while (iter.hasMore()) {
			Rover * pRover = iter.getNext();

			if (pRover->getSpeed() == speedType) {
				if ((pRover->collisionCount > COLLISION_LIMIT) || (pRover->stuckCount > STUCK_LIMIT)) {
					pRover->setColliding( false );
					pRover->setStuck( false );
					displacedRovers.append(pRover);
				} else if (pRover->collisionCount > 0) {
					pRover->rotate90Cw();
				}
			}
		}
	}

	bool occupied_cells[ ROVER_FIELD_ROWS ][ ROVER_FIELD_COLUMNS ];

	void recordOccupiedCells() {
		memset(occupied_cells, 0, sizeof(occupied_cells));

		DoubleLinkListIterator<Rover> rover_iter(activeRovers);
		while (rover_iter.hasMore()) {
			Rover * pRover = rover_iter.getNext();
			occupied_cells[ pRover->pos_row ][ pRover->pos_col ] = true;
		}

		DoubleLinkListIterator<Obstacle> obstacle_iter(activeObstacles);
		while (obstacle_iter.hasMore()) {
			Obstacle * pObstacle = obstacle_iter.getNext();
			occupied_cells[ pObstacle->pos_row ][ pObstacle->pos_col ] = true;
		}
	}

	void repositionDisplacedRovers() {

		int numDisplacedRovers = displacedRovers.getNumMembers();
		if (numDisplacedRovers > 0) {
			
			recordOccupiedCells();

			int numRows = matrix.dimensions.num_rows;
			int numColumns = matrix.dimensions.num_cols;
			int visibleCellCount = numRows * numColumns;
			for (int k = 0; k < numDisplacedRovers; k++) {
				Rover * pRover = displacedRovers.getAt(k);

				bool placed = false;

				int row    = int(random.generateZeroToOne() * float(numRows - 1));
				int column = int(random.generateZeroToOne() * float(numColumns - 1));

				row    = clamp(row, 0, numRows);
				column = clamp(column, 0, numColumns);

				for (int k = 0; k < visibleCellCount; k++) {

					if (! occupied_cells[row][column]) {
						// ASSIGN ROVER to this cell
						// DEBUG("Displaced rover placed at %d, %d", row, column);
						pRover->setPosition(row, column);
						placed = true;
						break;
					}
					column++; 
					if (column >= numColumns) {
						column = 0;
						row++;
						if (row >= numRows) {
							row = 0;
						}
					}

				}

				// if failed to assign, discard 
				if (! placed) {
					//DEBUG("Discarding displaced Rover .. no place for it");
					activeRovers.remove(pRover);
					roverPool.pushTail(pRover);
				}
			}

			displacedRovers.clear();
		}
	}

	void processGateClosures() {
		WallStrikeTimer * pWallStrikeTimer = wallStrikeTimerList.peekFront();

		if (pWallStrikeTimer) {
			pWallStrikeTimer->delay--;
			while (pWallStrikeTimer && pWallStrikeTimer->delay <= 0) {
	
				//DEBUG(" Gate Timer = %p, wall %d, pitch interval %d, delay %d", pWallStrikeTimer, pWallStrikeTimer->wallDirection, pWallStrikeTimer->pitchInterval_idx, pWallStrikeTimer->delay);
				//debug_printWallStrikeTimerList("processGateClosure");

				wallStrikeTimerList.popFront(); // consume the expired item

				//debug_printWallStrikeTimerList("processGateClosure - after pop");

				// process timeout 
				// TODO: use a table lookup or something instead of switch
				// walls[ wallDirection ].endStrike()
				switch (pWallStrikeTimer->wallDirection) {
					case WALL_NORTH:
						northWall.endStrike(pWallStrikeTimer->pitchInterval_idx);
						break;
					case WALL_EAST:
						eastWall.endStrike(pWallStrikeTimer->pitchInterval_idx);
						break;
					case WALL_SOUTH:
						southWall.endStrike(pWallStrikeTimer->pitchInterval_idx);
						break;
					case WALL_WEST:
						westWall.endStrike(pWallStrikeTimer->pitchInterval_idx);
						break;
					default:
						// TODO: log something 
						break;
				}

				// voiceUpdateRequired = true; 
		
				// Setting redraw required here causes UI to miss rover move frames .. why?
				// redrawRequired = true;

				pWallStrikeTimer = wallStrikeTimerList.peekFront();
			}
		}
	}

	void addWallStrikeTimer(WallDirection wallDirection, int pitch_interval_idx, long delaySamples) {
		WallStrikeTimer * pWallStrikeTimer = createWallStrikeTimer(wallDirection, pitch_interval_idx, delaySamples);
		wallStrikeTimerList.insertTimeOrdered(pWallStrikeTimer);
	}


//	void addOutputVoices(Wall & wall) { 
//		//wallOutputPitches[wall.direction].clear();
//		wallOutputPitches[wall.direction].clearActivePitches();
//
//		//DEBUG("addOutputVoices: wall %d, num visible blocks = %d", wall.direction, wall.numVisibleBlocks);
//		for (int block_idx = 0; block_idx < ROVER_FIELD_DIMENSION; block_idx++) {
//			if (wall.isStrikeActive(block_idx)) {
//				int pitch = wall.getPitchForBlock(block_idx);
//				polyOutputPitches.addPitch(pitch);
//				wallOutputPitches[wall.direction].addPitch(pitch);
//			}
//		}
//	}

	void addOutputVoices() {

		OutputPitches wallPitches;
		OutputPitches polyPitches;

		wallPitches.setPolyphony(MAX_POLYPHONY);
		polyPitches.setPolyphony(MAX_POLYPHONY);
		
		for (int k = 0; k < NUM_WALLS; k++) {
			wallPitches.clear();

			//DEBUG("addOutputVoices: wall %d, num visible blocks = %d", wall.direction, wall.numVisibleBlocks);
			for (int block_idx = 0; block_idx < ROVER_FIELD_DIMENSION; block_idx++) {
				if (walls[k]->isStrikeActive(block_idx)) {
					int pitch = walls[k]->getPitchForBlock(block_idx);
					wallPitches.addPitch(pitch);
					polyPitches.addPitch(pitch);
				}
			}

			if (wallPitches.getNumPitches() > 0) {
				wallOutputPitches[k].clearActivePitches();
				if (outputMode == OUTPUT_MODE_MINIMUM) {
					wallOutputPitches[k].addPitch(wallPitches.getMinimumPitch());
				} else if (outputMode == OUTPUT_MODE_MAXIMUM) {
					wallOutputPitches[k].addPitch(wallPitches.getMaximumPitch());
				} else {
					wallOutputPitches[k].clearActivePitches();
					for (int i = 0; i < wallPitches.getNumPitches(); i++) {
						wallOutputPitches[k].addPitch(wallPitches.pitches[i]);
					}
				}
			}
		} 

		if (polyPitches.getNumPitches() > 0) {
			if (outputMode == OUTPUT_MODE_MINIMUM) {
				polyOutputPitches.clear();
				polyOutputPitches.addPitch(polyPitches.getMinimumPitch());
			} else if (outputMode == OUTPUT_MODE_MAXIMUM) {
				polyOutputPitches.clear();
				polyOutputPitches.addPitch(polyPitches.getMaximumPitch());
			} else {
				polyOutputPitches.clearActivePitches();
				for (int i = 0; i < polyPitches.getNumPitches(); i++) {
					polyOutputPitches.addPitch(polyPitches.pitches[i]);
				}
			}
		}
	}

	void setPolyphony(int polyphony) { 
		polyOutputPitches.setPolyphony(polyphony);
		for (int k = 0; k < NUM_WALLS; k++) {
			wallOutputPitches[k].setPolyphony(polyphony);
		}
	}


// TODO: move up 
	struct GateVoctIds {
		int gateId;
		int voctId;
	};

// TODO: move up 
	GateVoctIds wallOutputIds[ NUM_WALLS ] = {
		{ GATE_NORTH_OUTPUT, VOCT_NORTH_OUTPUT },
		{ GATE_EAST_OUTPUT,  VOCT_EAST_OUTPUT },
		{ GATE_SOUTH_OUTPUT, VOCT_SOUTH_OUTPUT },
		{ GATE_WEST_OUTPUT,  VOCT_WEST_OUTPUT }
	};

	void produceOutput() {
		int activeStrikes = 0;

		for (int k = 0; k < NUM_WALLS; k++) {
			int gate_id = wallOutputIds[k].gateId;
			int voct_id = wallOutputIds[k].voctId;

			outputs[gate_id].setVoltage(walls[k]->strike_count > 0 ? 10.0 : 0.0);

			activeStrikes += walls[k]->strike_count;

			if (outputs[voct_id].isConnected()) {
				int pitch;
				float voct;
				if (outputMode == OUTPUT_MODE_MINIMUM) {
					outputs[voct_id].setChannels(1);
					pitch = wallOutputPitches[k].getMinimumPitch();
					voct = float(pitch - 48) / 12.0;  // +/- 5v
					voct = clamp(voct, -5.f, 5.f);
					outputs[voct_id].setVoltage(voct);
				}
				else if (outputMode == OUTPUT_MODE_MAXIMUM) {
					outputs[voct_id].setChannels(1);
					pitch = wallOutputPitches[k].getMaximumPitch();
					voct = float(pitch - 48) / 12.0;  // +/- 5v
					voct = clamp(voct, -5.f, 5.f);
					outputs[voct_id].setVoltage(voct);
				} else {
					int numPitches = wallOutputPitches[k].getNumPitches();
					outputs[voct_id].setChannels(numPitches);
					for (int c = 0; c < numPitches; c++) {
						int pitch = wallOutputPitches[k].getPitch(c);
						voct = float(pitch - 48) / 12.0;  // +/- 5v
						voct = clamp(voct, -5.f, 5.f);
						outputs[voct_id].setVoltage(voct, c);
					}
				}
			}
		}
		
		outputs[GATE_POLY_OUTPUT].setVoltage(activeStrikes > 0 ? 10.0 : 0.0);

		if (outputs[VOCT_POLY_OUTPUT].isConnected()) {
			int pitch;
			float voct;
			if (outputMode == OUTPUT_MODE_MINIMUM) {
				outputs[VOCT_POLY_OUTPUT].setChannels(1);
				pitch = polyOutputPitches.getMinimumPitch();
				voct = float(pitch - 48) / 12.0;  // +/- 5v
				voct = clamp(voct, -5.f, 5.f);
				outputs[VOCT_POLY_OUTPUT].setVoltage(voct);
			}
			else if (outputMode == OUTPUT_MODE_MAXIMUM) {
				outputs[VOCT_POLY_OUTPUT].setChannels(1);
				pitch = polyOutputPitches.getMaximumPitch();
				voct = float(pitch - 48) / 12.0;  // +/- 5v
				voct = clamp(voct, -5.f, 5.f);
				outputs[VOCT_POLY_OUTPUT].setVoltage(voct);
			}
			else {
				int numPitches = polyOutputPitches.getNumPitches();
				outputs[VOCT_POLY_OUTPUT].setChannels(numPitches);
				for (int c = 0; c < numPitches; c++) {
					pitch = polyOutputPitches.getPitch(c);
					voct = float(pitch - 48) / 12.0;  // +/- 5v
					voct = clamp(voct, -5.f, 5.f);
					outputs[VOCT_POLY_OUTPUT].setVoltage(voct, c);
					// DEBUG("ProduceOutput: channel %d, voltage %f", c, voct);
				}
			}
		}
	}

	void reverseAllRovers() {
		DoubleLinkListIterator<Rover> iter(activeRovers);
		while (iter.hasMore()) {
			Rover * pRover = iter.getNext();
			pRover->reverse();
		}
	}

	void restoreOlderSnapshot() {
		if (activeStackIndex > 0) {
			activeStackIndex--;
			restoreRoverFieldState(activeStackIndex);
		 	redrawRequired = true;
			// TODO: add activeStackIndex to something (???) to be displayed ?? 
		}
	}

	void restoreNewerSnapshot() {
		if (activeStackIndex < roverFieldStack.size() - 1) {
			activeStackIndex++;
			restoreRoverFieldState(activeStackIndex);
		 	redrawRequired = true;
			// TODO: add activeStackIndex to something (???) to be displayed ?? 
		}
	}

	void pushSnapshot() {
		// If stack is full, replace the top layer 
		if (roverFieldStack.isFull()) {
			popSnapshot();
			activeStackIndex --;
		}
		RoverFieldSnapshot * pSnapshot = roverFieldStack.allocateSnapshot();
		pSnapshot->addObstacles(activeObstacles);
		pSnapshot->addRovers(activeRovers);
		roverFieldStack.push(pSnapshot);
		activeStackIndex++;
	}
	
	void popSnapshot() {
		// If stack is not empty remove and discard the top layer 
		if (! roverFieldStack.isEmpty()) {
			RoverFieldSnapshot * pSnapshot = roverFieldStack.pop();
			roverFieldStack.retire(pSnapshot);			
		}
	}

	void restoreRoverFieldState(int idx) {
		RoverFieldSnapshot * pSnapshot = roverFieldStack.peekAt(idx);
		if (pSnapshot) {
			// discard active rovers 
			// discard active obstacles
			// clear matrix 
			// clear collisions 
			// clear whatever might be holding rover/obstacle state 

			DoubleLinkListIterator<RoverState> roverState_iter(pSnapshot->roverStates);
			while (roverState_iter.hasMore()) {
				RoverState * pRoverState = roverState_iter.getNext(); 
				Rover * pRover = allocateRover();
				pRoverState->populateRover(pRover);
				activeRovers.pushTail(pRover);

				// TODO: what other structs need to be updated ??
				//   void activateRover(pObstacle) 
			}

			DoubleLinkListIterator<ObstacleState> obstacleState_iter(pSnapshot->obstacleStates);
			while (obstacleState_iter.hasMore()) {
				ObstacleState * pObstacleState = obstacleState_iter.getNext(); 
				Obstacle * pObstacle = allocateObstacle();
				pObstacleState->populateObstacle(pObstacle);
				activeObstacles.pushTail(pObstacle);

				// TODO: what other structs need to be updated ??
				//   void activateObstacle(pObstacle) 
			}

		}
	}

	WallStrikeTimer * createWallStrikeTimer(WallDirection wallDirection, int pitchInterval_idx, long delaySamples) {
		WallStrikeTimer * pWallStrikeTimer = wallStrikeTimerPool.popFront();
		if (pWallStrikeTimer == NULL) {
			pWallStrikeTimer = new WallStrikeTimer;
		}
		pWallStrikeTimer->initialize(wallDirection, pitchInterval_idx, delaySamples);
		return pWallStrikeTimer;
	}

	void retireWallStrikeTimer(WallStrikeTimer * pWallStrikeTimer) {
		if (pWallStrikeTimer) { 
			wallStrikeTimerPool.pushTail(pWallStrikeTimer);
		}
	}

	RoverAction * createRoverAction(RoverActionType roverActionType, RoverType roverType, int row, int col) {
		RoverAction * pRoverAction = roverActionPool.popFront();
		if (pRoverAction == NULL) {
			pRoverAction = new RoverAction;
		}
		pRoverAction->initialize(roverActionType, roverType, row, col);
		return pRoverAction;
	}

	void retireRoverAction(RoverAction * pRoverAction) {
		if (pRoverAction) {
			roverActionPool.pushTail(pRoverAction);
		}
	}

	Rover * createRover(RoverType roverType, int row, int col) {
		Rover * pRover = allocateRover();

		pRover->initialize(roverType, row, col, getBirthday(), matrix.dimensions);
		return pRover;
	}

	Rover * allocateRover() {
		Rover * pRover = roverPool.popFront();
		if (pRover == NULL) {
			pRover = new Rover;
		}
		return pRover;
	}

	void retireRover(Rover * pRover) {
		if (pRover) {
			roverPool.pushTail(pRover);
		}
	}

	ObstacleAction * createObstacleAction(ObstacleActionType obstacleActionType, ObstacleType obstacleType, int row, int col) {
		ObstacleAction * pObstacleAction = obstacleActionPool.popFront();
		if (pObstacleAction == NULL) {
			pObstacleAction = new ObstacleAction;
		}
		pObstacleAction->initialize(obstacleActionType, obstacleType, row, col);
		return pObstacleAction;
	}

	void retireObstacleAction(ObstacleAction * pObstacleAction) {
		if (pObstacleAction) {
			obstacleActionPool.pushTail(pObstacleAction);
		}
	}

	Obstacle * createObstacle(ObstacleType obstacleType, int row, int col) {
		Obstacle * pObstacle = allocateObstacle();

		pObstacle->initialize(obstacleType, row, col, getBirthday(), matrix.dimensions);
		return pObstacle;
	}

	Obstacle * allocateObstacle() {
		Obstacle * pObstacle = obstaclePool.popFront();
		if (pObstacle == NULL) {
			pObstacle = new Obstacle;
		}
		return pObstacle;
	}

	void retireObstacle(Obstacle * pObstacle) {
		if (pObstacle) {
			obstaclePool.pushTail(pObstacle);
		}
	}


	int getBirthday() { 
		return activeRovers.size() + activeObstacles.size() + 1;
	}
};


struct RoverFieldCellIcon; 

struct IconPalette { 
	RoverFieldCellIcon * roverIcons[ NUM_ROVER_TYPES ];
	RoverFieldCellIcon * obstacleIcons[ NUM_OBSTACLE_TYPES ];
	OccupantSelector selector;

	IconPalette() {
		memset(roverIcons, 0, sizeof(roverIcons));
		memset(obstacleIcons, 0, sizeof(obstacleIcons));
	}

	void select(UiRoverFieldCell & roverFieldCell);

	void refreshSelected();
}; 


struct RoverFieldCellIcon : ParamWidget { 
	UiRoverFieldCell fieldCell;
	std::shared_ptr<Svg> svg;
	IconPalette * pIconPalette;
	bool selected; 

	const NVGcolor COLOR_SELECTED = nvgRGB(59, 149, 196);

	RoverFieldCellIcon() { 
		pIconPalette = 0;
		selected = false;
	}

	void setIconPalette(IconPalette & iconPalette) { 
		pIconPalette = &iconPalette;
	}

	void setSelected(bool beSelected) {
		selected = beSelected;
	}

	void setObstacleType(ObstacleType obstacleType) {
		fieldCell.setObstacleType(obstacleType);
	}
	
	void setRoverType(RoverType roverType, SpeedType speedType) {
		fieldCell.setRoverType(roverType, speedType);
	}

	void wrap() {
		if (svg && svg->handle) {
			box.size = math::Vec(svg->handle->width, svg->handle->height);
		}
		else {
			box.size = math::Vec();
		}
	}

	void setSvg(std::shared_ptr<Svg> svg) {
		this->svg = svg;
		wrap();
	}

	void onButton(const event::Button& e) override {
		if (e.action == GLFW_PRESS) {
			//DEBUG("Obstacle: BUTTON button = 0x%x, pos = %f, %f", e.button, e.pos.x, e.pos.y);
			math::Vec c = box.size.div(2);
			float dist = e.pos.minus(c).norm();
			//DEBUG("Obstacle: BUTTON   dist = %f, my pos = %f, %f", dist, box.pos.x, box.pos.y);
			//DEBUG("Obstacle: BUTTON              my size = %f, %f", box.size.x, box.size.y);
			if (dist <= c.x) {
				//DEBUG("Obstacle: BUTTON -- icon obs(%d) rover(%d) is selected", fieldCell.getObstacleType(), fieldCell.getRoverType());
				OpaqueWidget::onButton(e);

				if (pIconPalette) {
					pIconPalette->select(fieldCell);
				}
			}
			e.consume(this);
		} else {
			//DEBUG("Obstacle: BUTTON button = 0x%x, pos = %f, %f -- RELEASE", e.button, e.pos.x, e.pos.y);		
		}
	}


	virtual void draw(const DrawArgs& args) override {
			nvgSave(args.vg);
			svgDraw(args.vg, svg->handle);
			if (selected) {
				// highlight the icon border 
				nvgBeginPath(args.vg);
				nvgRect(args.vg, -2, -2, box.size.x + 4, box.size.y + 4);
				nvgClosePath(args.vg);
				nvgStrokeColor(args.vg, COLOR_SELECTED);
				nvgStrokeWidth(args.vg, 2.25);
				nvgStroke(args.vg);
			}
			nvgRestore(args.vg);
	}

};

void IconPalette::select(UiRoverFieldCell & roverFieldCell) { 
	if (roverFieldCell.isRover()) {
		selector.selectRoverType(roverFieldCell.getRoverType());
	} else {
		selector.selectObstacleType(roverFieldCell.getObstacleType());
	}
	refreshSelected();
}

void IconPalette::refreshSelected() { 
	// TODO: consider setting all to false, then using index to set specific one selected 
	for (int k = 0; k < NUM_ROVER_TYPES; k++) {
		roverIcons[k]->setSelected(selector.isRover() && selector.getRoverType() == roverIcons[k]->fieldCell.getRoverType());
	}
	for (int k = 0; k < NUM_OBSTACLE_TYPES; k++) {
		obstacleIcons[k]->setSelected(selector.isObstacle() && selector.getObstacleType() == obstacleIcons[k]->fieldCell.getObstacleType());
	}
}


struct RoverAreaWidget : rack::widget::OpaqueWidget {
	//rack::widget::FramebufferWidget * fb;

	Traveler * pModule;
	IconPalette * pIconPalette;

	std::shared_ptr<rack::Svg> roverImageTable[NUM_SPEED_TYPES][ NUM_ROVER_TYPES ]; 
	std::shared_ptr<rack::Svg> obstacleImageTable[ NUM_OBSTACLE_TYPES ]; 

	std::shared_ptr<rack::Svg> pRoverCollisionCellImage;
	std::shared_ptr<rack::Svg> pRoverEmptyCellImage;

	float six_mm_pixels;
	float label_offset_left;
	float label_offset_upper;
	float label_offset_lower_left;
	float label_offset_lower_bottom;

	int updateCount; // compare to module->uiUpdateCounter to detect time to update matrix display 

	int draw_counter; 

	const NVGcolor COLOR_BLACK = nvgRGB(0, 0, 0);
	const NVGcolor strikeColor = nvgRGB(255,0,0);
	const NVGcolor lowRange  = nvgHSL(30./360., 0.66, 0.66);
	const NVGcolor highRange = nvgHSL(200./360., 0.66, 0.66);

	RoverAreaWidget() {
		
		//six_mm_pixels = mm2px(6.0);
		six_mm_pixels = 17.5f; //

		box.size.x = six_mm_pixels * 16;	// Set this when widget is created 
		box.size.y = six_mm_pixels * 16;		

		// Label offsets
		label_offset_left  = (six_mm_pixels * 0.2);
		label_offset_upper = (six_mm_pixels * 0.490);
		label_offset_lower_left   = (six_mm_pixels * 0.525);
		label_offset_lower_bottom = (six_mm_pixels * 0.925);

		// TODO: consider making the image tables owned by the main widget

		roverImageTable[ SPEED_NORMAL ][ ROVER_NORTH     ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-n.svg"));
		roverImageTable[ SPEED_NORMAL ][ ROVER_NORTHEAST ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-ne.svg"));
		roverImageTable[ SPEED_NORMAL ][ ROVER_EAST      ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-e.svg"));
		roverImageTable[ SPEED_NORMAL ][ ROVER_SOUTHEAST ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-se.svg"));
		roverImageTable[ SPEED_NORMAL ][ ROVER_SOUTH     ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-s.svg"));
		roverImageTable[ SPEED_NORMAL ][ ROVER_SOUTHWEST ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-sw.svg"));
		roverImageTable[ SPEED_NORMAL ][ ROVER_WEST      ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-w.svg"));
		roverImageTable[ SPEED_NORMAL ][ ROVER_NORTHWEST ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-nw.svg"));

		roverImageTable[ SPEED_ALT ][ ROVER_NORTH     ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-n-slow.svg"));
		roverImageTable[ SPEED_ALT ][ ROVER_NORTHEAST ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-ne-slow.svg"));
		roverImageTable[ SPEED_ALT ][ ROVER_EAST      ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-e-slow.svg"));
		roverImageTable[ SPEED_ALT ][ ROVER_SOUTHEAST ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-se-slow.svg"));
		roverImageTable[ SPEED_ALT ][ ROVER_SOUTH     ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-s-slow.svg"));
		roverImageTable[ SPEED_ALT ][ ROVER_SOUTHWEST ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-sw-slow.svg"));
		roverImageTable[ SPEED_ALT ][ ROVER_WEST      ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-w-slow.svg"));
		roverImageTable[ SPEED_ALT ][ ROVER_NORTHWEST ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-nw-slow.svg"));

		obstacleImageTable[ OBSTACLE_MIRROR_HORIZONTAL     ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-horiz-mirror.svg"));
		obstacleImageTable[ OBSTACLE_MIRROR_HORIZONTAL_FLIP] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-horiz-mirror-toggle.svg"));
   		obstacleImageTable[ OBSTACLE_MIRROR_VERTICAL       ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-vertical-mirror.svg"));
   		obstacleImageTable[ OBSTACLE_MIRROR_VERTICAL_FLIP  ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-vertical-mirror-toggle.svg"));
   		obstacleImageTable[ OBSTACLE_MIRROR_SQUARE         ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-mirror-square.svg"));
   		obstacleImageTable[ OBSTACLE_MIRROR_DIAMOND        ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-mirror-diamond.svg"));
   		obstacleImageTable[ OBSTACLE_MIRROR_NE_SW          ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-ne-sw-mirror.svg"));
   		obstacleImageTable[ OBSTACLE_MIRROR_NE_SW_FLIP     ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-ne-sw-mirror-toggle.svg"));
   		obstacleImageTable[ OBSTACLE_MIRROR_NW_SE          ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-nw-se-mirror.svg"));
   		obstacleImageTable[ OBSTACLE_MIRROR_NW_SE_FLIP     ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-nw-se-mirror-toggle.svg"));
		obstacleImageTable[ OBSTACLE_WEDGE_NORTH           ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-wedge-north.svg"));
		obstacleImageTable[ OBSTACLE_WEDGE_EAST            ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-wedge-east.svg"));
		obstacleImageTable[ OBSTACLE_WEDGE_SOUTH           ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-wedge-south.svg"));
		obstacleImageTable[ OBSTACLE_WEDGE_WEST            ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-wedge-west.svg"));
		obstacleImageTable[ OBSTACLE_DETOUR                ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-detour.svg"));
		obstacleImageTable[ OBSTACLE_WORMHOLE              ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-spiral.svg"));
		obstacleImageTable[ OBSTACLE_SPIN                  ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-sprocket.svg"));
		obstacleImageTable[ OBSTACLE_SPEED_NORMAL          ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-speed-normal.svg"));
		obstacleImageTable[ OBSTACLE_SPEED_ALT             ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-speed-alt.svg"));
		obstacleImageTable[ OBSTACLE_SPEED_TOGGLE          ] = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/obstacle-speed-toggle.svg"));

// NOTE: the dice icon is not used yet 

		pRoverCollisionCellImage = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-collision.svg"));
		pRoverEmptyCellImage = APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/rover-empty-cell.svg"));

// TODO: use frame buffer ?
//		fb = new widget::FramebufferWidget;
//		fb->box.size = box.size;
//		addChild(fb);

		updateCount = -1; // init to something different than module so field draws first time
		pModule = NULL;
		pIconPalette = NULL;

		draw_counter = 0; // Debug: counter for print statements
	}

	void setModule(Traveler * pModule) {
		this->pModule = pModule;
	}

	void setIconPalette(IconPalette & iconPalette) {
		this->pIconPalette = & iconPalette;
	}

	virtual void onButton(const event::Button &e) override {

		if (e.action == GLFW_PRESS) {
			//DEBUG("! -- BUTTON !");
			//DEBUG(" event pos: %f, %f", e.pos.x, e.pos.y);

			//	/** GLFW_MOUSE_BUTTON_LEFT, GLFW_MOUSE_BUTTON_RIGHT, GLFW_MOUSE_BUTTON_MIDDLE, etc. */
			//	int button;
			//	/** GLFW_PRESS or GLFW_RELEASE */
			//	int action;
			//	/** GLFW_MOD_* */
			//	int mods;
			//#define GLFW_MOD_SHIFT           0x0001
			//#define GLFW_MOD_CONTROL         0x0002
			//#define GLFW_MOD_ALT             0x0004
			//#define GLFW_MOD_SUPER           0x0008
			//#define GLFW_MOD_CAPS_LOCK       0x0010
			//#define GLFW_MOD_NUM_LOCK        0x0020

			//e.mods;

			//DEBUG("box pos x,y = %f, %f", box.pos.x, box.pos.y);
			//DEBUG("box size x,y = %f, %f", box.size.x, box.size.y);

			int row = e.pos.y / six_mm_pixels; 
			int col = e.pos.x / six_mm_pixels; 
			
			//DEBUG(" Cell row %d, col %d  (mods = 0x%x)", row, col, e.mods);
			
			// TODO: check that cell is inside active range
			// if row, col is inside active matrix 
			// if uiMatrix.isCellVisible(cell_id)
			//     .... 
			// TODO: if adding rovers this way, create a queue with RoverActions. UI can post, main can pull and apply
			// module->addRover(cell_id, type)

			if (e.button == GLFW_MOUSE_BUTTON_LEFT) { 
				//DEBUG("-- TODO: Add occupant at row %d, col %d", row, col);
				if (pIconPalette) {
					if (pIconPalette->selector.isRover()) {
						pModule->enqueueRoverAction(ROVER_ACTION_ADD, pIconPalette->selector.getRoverType(), row, col);
					} else {
						pModule->enqueueObstacleAction(OBSTACLE_ACTION_ADD, pIconPalette->selector.getObstacleType(), row, col);
					}
				}
			} else if (e.button == GLFW_MOUSE_BUTTON_RIGHT) { 
				//DEBUG("-- TODO: Clear cell at row %d, col %d", row, col);
				pModule->enqueueRoverAction(ROVER_ACTION_REMOVE, pIconPalette->selector.getRoverType(), row, col);
				pModule->enqueueObstacleAction(OBSTACLE_ACTION_REMOVE, pIconPalette->selector.getObstacleType(), row, col);
			}
			// ignore middle button


//			int mods = e.mods & 0xFFFF; // ignore CAPS lock and NUM Lock 
//
//			if (e.button == GLFW_MOUSE_BUTTON_LEFT) { 
//				if (mods == 0) {
//					pModule->enqueueRoverAction(ROVER_ACTION_ADD, ROVER_NORTH, row, col);
//				} else if (mods & GLFW_MOD_SHIFT) {
//					pModule->enqueueRoverAction(ROVER_ACTION_ADD, ROVER_EAST, row, col);
//				} else if (mods & GLFW_MOD_CONTROL) {
//					pModule->enqueueRoverAction(ROVER_ACTION_ADD, ROVER_SOUTH, row, col);
//				} else if (mods & GLFW_MOD_ALT) {
//					pModule->enqueueRoverAction(ROVER_ACTION_ADD, ROVER_WEST, row, col);
//				}
//			} else if (e.button == GLFW_MOUSE_BUTTON_RIGHT) { 
//				if (mods == 0) {
//					pModule->enqueueRoverAction(ROVER_ACTION_ADD, ROVER_NORTHEAST, row, col);
//				} else if (mods & GLFW_MOD_SHIFT) {
//					pModule->enqueueRoverAction(ROVER_ACTION_ADD, ROVER_SOUTHEAST, row, col);
//				} else if (mods & GLFW_MOD_CONTROL) {
//					pModule->enqueueRoverAction(ROVER_ACTION_ADD, ROVER_SOUTHWEST, row, col);
//				} else if (mods & GLFW_MOD_ALT) {
//					pModule->enqueueRoverAction(ROVER_ACTION_ADD, ROVER_NORTHWEST, row, col);
//				}
//
//			}
			
			// else if GLFW_MOUSE_BUTTON_MIDDLE ??

		}

		e.stopPropagating();
		// Consume if not consumed by child
		if (!e.isConsumed()) {
			e.consume(this);
		}
	}



 	void draw(const DrawArgs& args) override {

		 if (pModule == NULL) {
			 return;
		 }

		int moduleProducerCount = pModule->producerCount;
		if (moduleProducerCount != pModule->consumerCount) {
			updateCount = moduleProducerCount;
			// DEBUG(" Draw: count %d", updateCount);			
		}

		UiMatrix * pUiMatrix = pModule->pConsumer;

		int num_rows = pUiMatrix->numRowsActive;
		int num_cols = pUiMatrix->numColumnsActive;

		std::shared_ptr<rack::Svg> imagePtr;

		nvgTextAlign(args.vg, NVG_ALIGN_LEFT);

		// DRAW Matrix 

		for (int row = 0; row < num_rows; row++) {
			float row_offset = row * six_mm_pixels;
			for (int col = 0; col < num_cols; col++) {

				UiRoverFieldCell * pCell = & pUiMatrix->cells[ row ][ col ];
				CellContentType cell_type = pCell->contentType;

				imagePtr = pRoverEmptyCellImage;

				if (cell_type == CELL_ROVER) {
					imagePtr = getRoverImage(pCell->getRoverType(), pCell->getRoverSpeed());
				}
				else if (cell_type == CELL_OBSTACLE) {
					imagePtr = getObstacleImage(pCell->getObstacleType());
				}		
				else if (cell_type == CELL_COLLISION) {
					imagePtr = pRoverCollisionCellImage;
				}

				//else if (cell_type == CELL_EMPTY) {
				//	imagePtr = pRoverEmptyCellImage;
				//}
				//if (imagePtr == NULL) {
				//}

				nvgSave(args.vg);
				nvgTranslate(args.vg, col * six_mm_pixels, row_offset);
				svgDraw(args.vg, imagePtr->handle);
				nvgRestore(args.vg);
			}
		}		

	 	drawWall(args, pUiMatrix->walls[WALL_NORTH], 0, -six_mm_pixels, num_cols, true); // horizontal
	 	drawWall(args, pUiMatrix->walls[WALL_EAST],  num_cols * six_mm_pixels, 0, num_rows, false); // vertical
	 	drawWall(args, pUiMatrix->walls[WALL_SOUTH], 0, num_rows * six_mm_pixels, num_cols, true); // horizontal
	 	drawWall(args, pUiMatrix->walls[WALL_WEST],  -six_mm_pixels, 0, num_rows, false); // vertical

 		pModule->consumerCount = moduleProducerCount;  // acknowledge done with buffer 
	}


	std::shared_ptr<rack::Svg> getRoverImage(RoverType roverType, SpeedType speedType) {
		return roverImageTable[ int(speedType) ][ int(roverType) ];
	}

	std::shared_ptr<rack::Svg> getObstacleImage(ObstacleType obstacleType) {
		return obstacleImageTable[ int(obstacleType) ];
	}

 	void drawWall(const DrawArgs& args, UiWall & wall, float x, float y, int num_blocks, bool isHorizontal) {
		 
		// DEBUG("Draw Wall: num_blocks %d, horiz %d", num_blocks, isHorizontal);

		float col_increment = (isHorizontal ? six_mm_pixels : 0.f);
		float row_increment = (isHorizontal ? 0.f : six_mm_pixels);
		
		int lowestPitch = wall.lowestPitch;
		int highestPitch = wall.highestPitch;

		float pitchRange = (highestPitch - lowestPitch);
		if (pitchRange < 1.0) {
			pitchRange = 1.0;
		}

		NVGcolor pitchColor;

		for (int block_idx = 0; block_idx < num_blocks; block_idx++) {

			int pitch = wall.blocks[block_idx].pitch;

			if (wall.blocks[block_idx].isCollision()) {
				pitchColor = strikeColor;
			} else {
				float pitchDepth = float(pitch - lowestPitch) / pitchRange;
				float octave = pitch / 12;
				// TODO: make a table of these colors, index by pitch _interval_
				float h = (30. + pitchDepth * (200.f - 30.f)) / 360.f;
				float s = 0.30 + (float(octave) / 10.f) * (.85f - .30f);
				float l = 0.66;
				pitchColor = nvgHSL(h, s, l);
			}
			drawWallBlock(args, x + (block_idx * col_increment), y + (block_idx * row_increment), pitch, pitchColor);
		}
	}

	void drawWallBlock(const DrawArgs& args, float x, float y, int pitch, NVGcolor pitchColor) {

		nvgSave(args.vg);
		nvgTranslate(args.vg, x, y);

		// cell fill 
		nvgBeginPath(args.vg);
		nvgRect(args.vg, 0, 0, six_mm_pixels, six_mm_pixels);
		nvgClosePath(args.vg);
		nvgFillColor(args.vg, pitchColor);
		nvgFill(args.vg);

		// Cell border
		nvgStrokeColor(args.vg, COLOR_BLACK);
		nvgStrokeWidth(args.vg, 0.5);
		nvgStroke(args.vg);

		// Pitch Label 
		nvgFillColor(args.vg, COLOR_BLACK);
		nvgFontSize(args.vg, 10);
		nvgText(args.vg, label_offset_left, label_offset_upper, noteNames[pitch % 12].c_str(), NULL);
		nvgFontSize(args.vg, 8);
		nvgText(args.vg, label_offset_lower_left, label_offset_lower_bottom, octaveNames[pitch / 12].c_str(), NULL);

		nvgRestore(args.vg);
	}

};

RoverAreaWidget * createRoverAreaWidget(Vec pos, Vec size) {
	RoverAreaWidget * pWidget = createWidget<RoverAreaWidget>(pos);
	//// pWidget->setSize(size);
	return pWidget;
}




template <class MODULE>
struct WritableLabel : Label {
	MODULE * pModule;

	WritableLabel() {
		fontSize = 14;
		color = nvgRGB(0, 0, 0);
		text = "?";
	}

	void setModule(MODULE * pModule) {
		this->pModule = pModule;
	}

	void draw(const DrawArgs& args) override {
		prepareToDraw(args);
		Label::draw(args);
	}

	virtual void prepareToDraw(const DrawArgs& args) {};
};

struct ScaleLabel : WritableLabel<Traveler> {
	Traveler::ScaleId scaleId;

	ScaleLabel() {
		fontSize = 12;
		// alignment = CENTER_ALIGNMENT; // TODO: set the box size first 
	}

	void prepareToDraw(const DrawArgs& args) override {
		if (pModule && text != pModule->scales[scaleId].scaleTemplate.name) {
			text = pModule->scales[scaleId].scaleTemplate.name;
		}
	} 
};

struct RootNoteLabel : WritableLabel<Traveler> {
	Traveler::ScaleId scaleId;
	int rootNote = -1; 

	RootNoteLabel() {
		fontSize = 12;
		// alignment = CENTER_ALIGNMENT; // TODO: set the box size first 
	}

	void prepareToDraw(const DrawArgs& args) override {
		if (pModule && pModule->scales[scaleId].rootNote != rootNote) {
			rootNote = pModule->scales[scaleId].rootNote;
			int offset = rootNote % 12; 
			int octave = rootNote / 12;
			text = ::noteNames[offset] + ::octaveNames[octave];
		}
	} 
};

struct BpmLabel : WritableLabel<Traveler> {
	float bpm;
    char formattedValue[24];

	BpmLabel() {
		fontSize = 12;
		bpm = 0.f;
		// alignment = CENTER_ALIGNMENT; // TODO: set the box size first 
	}

	void prepareToDraw(const DrawArgs& args) override {
		if (pModule) { 
			float activeBpm; 

			if (pModule->clock.isConnectedToExternalClock()) { 
				activeBpm = -1;
				if (bpm != activeBpm) {
					bpm = activeBpm;
					text = "external";
				}
			} else {
				activeBpm = pModule->activeBpm;
				if (bpm != activeBpm) {
					bpm = activeBpm;
					sprintf(formattedValue, "%.2f", activeBpm);
					text = formattedValue;
				}
			}
		}
	} 
};

// TODO: delete commented code
/*********************
struct WallRootLabel : WritableLabel<Traveler> {
	int rootNote = -1; 
	int wallId = 0;

	WallRootLabel() {
		fontSize = 12;
	}

	void prepareToDraw(const DrawArgs& args) override {
		if (pModule) {
			int wallRootNote = pModule->walls[wallId]->scale.rootPitch;
			if (wallRootNote != rootNote) { 
				rootNote = wallRootNote; 
				text = ::noteNames[rootNote % 12] + ::octaveNames[rootNote / 12];
			}
		}
	} 
};
********************************/

// TODO: move this to library 
template <class MODULE>
struct SelectScaleFile : MenuItem
{
	MODULE * module;
	Traveler::ScaleId scaleId; 

	void onAction(const event::Action &e) override
	{
		const std::string dir = "";
		char *path = osdialog_file(OSDIALOG_OPEN, dir.c_str(), NULL, NULL);

		if(path)
		{
			module->setScaleFile(scaleId, path);
			// module->setScalaFile(path);
		    //  module->sample.load(path);
			//  module->root_dir = std::string(path);
			//  module->loaded_filename = module->sample.filename;
			free(path);
		}
	}
};

struct PolyphonyMenu : MenuItem {
	Traveler * module;

    struct PolyphonySubItem : MenuItem {
		Traveler * module;
		int polyphony;
		void onAction(const event::Action& e) override {
 			module->setActivePolyphony(polyphony);
		}
	};

	Menu *createChildMenu() override {
		Menu *menu = new Menu;

		std::string polyphonyNames[15] = {"2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16"};
		for (int i = 0; i < 15; i++) {
            int polyphony = std::stoi(polyphonyNames[i]);
			PolyphonySubItem * polySubItem = createMenuItem<PolyphonySubItem>(polyphonyNames[i]);
			polySubItem->rightText = CHECKMARK(module->activePolyphony == polyphony);
			polySubItem->module = module;
			polySubItem->polyphony = polyphony;
			menu->addChild(polySubItem);
		}

		return menu;
	}

};


/********************** TODO: consider table for icon placement positions *******************
struct ObstacleIconPosition {
	ObstacleType obstacleType;
	int row; 
	int col;
}; 

const ObstacleIconPosition [ NUM_OBSTACLE_TYPES ] = {
	{ OBSTACLE_MIRROR_HORIZONTAL,      0, 0 },
	{ OBSTACLE_MIRROR_HORIZONTAL_FLIP, 0, 1 },
	{ OBSTACLE_MIRROR_VERTICAL,        1, 0 },
	{ OBSTACLE_MIRROR_VERTICAL_FLIP,   1, 1 },
    { OBSTACLE_MIRROR_SQUARE,          2, 0},
    { OBSTACLE_MIRROR_DIAMOND,         2, 1},
    { OBSTACLE_MIRROR_NE_SW,           3, 0},
    { OBSTACLE_MIRROR_NE_SW_FLIP,      3, 1},
    { OBSTACLE_MIRROR_NW_SE,           4, 0},
    { OBSTACLE_MIRROR_NW_SE_FLIP,      4, 1},
	{ OBSTACLE_WEDGE_NORTH,            5, 0},
	{ OBSTACLE_WEDGE_EAST,             5, 1},
	{ OBSTACLE_WEDGE_SOUTH,            6, 0},
	{ OBSTACLE_WEDGE_WEST,             6, 1},
	{ OBSTACLE_DETOUR,                 7, 0},
	{ OBSTACLE_WORMHOLE,               7, 1},
	{ OBSTACLE_SPIN,                   7, 2},
	{ OBSTACLE_SPEED_NORMAL,           9, 0},
	{ OBSTACLE_SPEED_ALT,              9, 2},
	{ OBSTACLE_SPEED_TOGGLE,           9, 1}  // << toggle in the middle 
};
************************************************/



struct TravelerWidget : ModuleWidget {

	IconPalette iconPalette; 

	TravelerWidget(Traveler* module) {
		setModule(module);
		setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/traveler/Traveler.svg")));

		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
		addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
		addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.639, 10.85)), module, Traveler::RUN_RATE_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(214.708, 11.436)), module, Traveler::WALL_PROBABILITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(227.728, 11.436)), module, Traveler::OBSTACLE_PROBABILITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(240.637, 11.436)), module, Traveler::NOTE_PROBABILITY_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(253.447, 11.436)), module, Traveler::STEADY_PROBABILITY_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(285.05, 15.183)), module, Traveler::CLEAR_OBSTACLES_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(38.925, 21.066)), module, Traveler::RUN_BUTTON_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(54.127, 21.066)), module, Traveler::REVERSE_BUTTON_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(214.708, 21.437)), module, Traveler::WALL_PROBABILITY_ATTENUVERTER_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(227.634, 21.437)), module, Traveler::OBSTACLE_PROBABILITY_ATTENUVERTER_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(240.543, 21.437)), module, Traveler::NOTE_PROBABILITY_ATTENUVERTER_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(253.353, 21.437)), module, Traveler::STEADY_PROBABILITY_ATTENUVERTER_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(285.05, 23.541)), module, Traveler::CLEAR_ROVERS_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(285.05, 31.794)), module, Traveler::UNDO_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(14.501, 39.159)), module, Traveler::NOTE_LENGTH_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.973, 39.159)), module, Traveler::NOTE_LENGTH_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(269.057, 49.892)), module, Traveler::SCALE_INVERT_WEST_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(226.629, 49.893)), module, Traveler::SCALE_INVERT_NORTH_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(240.021, 49.893)), module, Traveler::SCALE_INVERT_SOUTH_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(254.703, 49.893)), module, Traveler::SCALE_INVERT_EAST_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(14.501, 53.027)), module, Traveler::ROW_COUNT_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.973, 53.027)), module, Traveler::ROW_COUNT_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(55.106, 58.379)), module, Traveler::HERD_BUTTON_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(226.629, 59.686)), module, Traveler::SHIFT_NORTH_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(240.021, 59.686)), module, Traveler::SHIFT_SOUTH_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(254.703, 59.686)), module, Traveler::SHIFT_EAST_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(269.057, 59.686)), module, Traveler::SHIFT_WEST_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(14.501, 65.482)), module, Traveler::COLUMN_COUNT_ATTENUVERTER_PARAM));
		addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(23.973, 65.482)), module, Traveler::COLUMN_COUNT_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(286.135, 65.565)), module, Traveler::RESET_WALLS_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(226.629, 70.705)), module, Traveler::ROTATE_NORTH_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(240.021, 70.705)), module, Traveler::ROTATE_SOUTH_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(254.703, 70.705)), module, Traveler::ROTATE_EAST_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(269.057, 70.705)), module, Traveler::ROTATE_WEST_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(226.629, 81.154)), module, Traveler::CLIMB_NORTH_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(240.021, 81.154)), module, Traveler::CLIMB_SOUTH_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(254.703, 81.154)), module, Traveler::CLIMB_EAST_PARAM));
		addParam(createParamCentered<RoundSmallBlackKnob>(mm2px(Vec(269.057, 81.154)), module, Traveler::CLIMB_WEST_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(285.806, 81.475)), module, Traveler::OUTPUT_MODE_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(6.529, 81.855)), module, Traveler::SELECT_SCALE_1_BUTTON_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(55.624, 81.855)), module, Traveler::SCALE_1_ROOT_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(6.529, 90.634)), module, Traveler::SELECT_SCALE_2_BUTTON_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(55.624, 90.634)), module, Traveler::SCALE_2_ROOT_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(226.629, 92.102)), module, Traveler::MUTE_NORTH_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(240.021, 92.102)), module, Traveler::MUTE_SOUTH_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(254.703, 92.102)), module, Traveler::MUTE_EAST_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(269.057, 92.102)), module, Traveler::MUTE_WEST_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(6.529, 99.192)), module, Traveler::SELECT_SCALE_3_BUTTON_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(55.624, 99.192)), module, Traveler::SCALE_3_ROOT_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(6.529, 107.705)), module, Traveler::SELECT_SCALE_4_BUTTON_PARAM));
		addParam(createParamCentered<Trimpot>(mm2px(Vec(55.624, 107.705)), module, Traveler::SCALE_4_ROOT_PARAM));
		addParam(createParamCentered<LEDButton>(mm2px(Vec(46.687, 116.899)), module, Traveler::ALT_SPEED_MODE_PARAM));

		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(8.523, 10.85)), module, Traveler::CLOCK_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(38.925, 10.85)), module, Traveler::RUN_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(54.127, 10.85)), module, Traveler::REVERSE_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(214.708, 30.748)), module, Traveler::WALL_PROBABILITY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(227.634, 30.748)), module, Traveler::OBSTACLE_PROBABILITY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(240.543, 30.748)), module, Traveler::NOTE_PROBABILITY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(253.353, 30.748)), module, Traveler::STEADY_PROBABILITY_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.529, 39.159)), module, Traveler::NOTE_LENGTH_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.529, 53.027)), module, Traveler::ROW_COUNT_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.529, 65.482)), module, Traveler::COLUMN_COUNT_CV_INPUT));
		addInput(createInputCentered<PJ301MPort>(mm2px(Vec(6.529, 117.111)), module, Traveler::SCALE_SELECTOR_CV_INPUT));

		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(226.794, 103.543)), module, Traveler::GATE_NORTH_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(240.11, 103.543)), module, Traveler::GATE_SOUTH_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(254.767, 103.543)), module, Traveler::GATE_EAST_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(269.056, 103.543)), module, Traveler::GATE_WEST_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(286.048, 103.543)), module, Traveler::GATE_POLY_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(226.794, 116.429)), module, Traveler::VOCT_NORTH_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(240.11, 116.429)), module, Traveler::VOCT_SOUTH_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(254.767, 116.429)), module, Traveler::VOCT_EAST_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(269.056, 116.429)), module, Traveler::VOCT_WEST_OUTPUT));
		addOutput(createOutputCentered<PJ301MPort>(mm2px(Vec(286.048, 116.429)), module, Traveler::VOCT_POLY_OUTPUT));

		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(38.925, 21.066)), module, Traveler::RUN_LED_LIGHT));
		addChild(createLightCentered<MediumLight<BlueLight>>(mm2px(Vec(54.127, 21.066)), module, Traveler::REVERSE_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(269.057, 49.892)), module, Traveler::SCALE_INVERT_WEST_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(226.629, 49.893)), module, Traveler::SCALE_INVERT_NORTH_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(240.021, 49.893)), module, Traveler::SCALE_INVERT_SOUTH_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(254.703, 49.893)), module, Traveler::SCALE_INVERT_EAST_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(55.106, 58.379)), module, Traveler::HERD_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(6.529, 81.855)), module, Traveler::SCALE_1_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(285.806, 86.767)), module, Traveler::OUTPUT_MODE_MINIMUM_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(6.529, 90.634)), module, Traveler::SCALE_2_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(285.806, 91.297)), module, Traveler::OUTPUT_MODE_MAXIMUM_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(226.629, 92.102)), module, Traveler::MUTE_NORTH_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(240.021, 92.102)), module, Traveler::MUTE_SOUTH_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(254.703, 92.102)), module, Traveler::MUTE_EAST_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(269.057, 92.102)), module, Traveler::MUTE_WEST_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(285.806, 95.209)), module, Traveler::OUTPUT_MODE_POLY_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(6.529, 99.192)), module, Traveler::SCALE_3_LED_LIGHT));
		addChild(createLightCentered<MediumLight<GreenLight>>(mm2px(Vec(6.529, 107.705)), module, Traveler::SCALE_4_LED_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(52.385, 116.899)), module, Traveler::ALT_SPEED_HALF_LIGHT));
		addChild(createLightCentered<MediumLight<RedLight>>(mm2px(Vec(57.553, 116.899)), module, Traveler::ALT_SPEED_DOUBLE_LIGHT));

		// Root note, scale, octave labels 
		float left; 
		float top; 

		ScaleLabel * pScaleLabel;
		RootNoteLabel * pRootNoteLabel;

		left = 10.f;
		top = 79.f;

		pScaleLabel = createWidget<ScaleLabel>(mm2px(Vec(left, top)));
		pScaleLabel->setModule(module); 
		pScaleLabel->scaleId = Traveler::ScaleId::SCALE_1;
		addChild(pScaleLabel);

		pScaleLabel = createWidget<ScaleLabel>(mm2px(Vec(left, top + (8.5f * 1.f))));
		pScaleLabel->setModule(module); 
		pScaleLabel->scaleId = Traveler::ScaleId::SCALE_2;
		addChild(pScaleLabel);

		pScaleLabel = createWidget<ScaleLabel>(mm2px(Vec(left, top + (8.5f * 2.f))));
		pScaleLabel->setModule(module); 
		pScaleLabel->scaleId = Traveler::ScaleId::SCALE_3;
		addChild(pScaleLabel);

		pScaleLabel = createWidget<ScaleLabel>(mm2px(Vec(left, top + (8.5f * 3.f))));
		pScaleLabel->setModule(module); 
		pScaleLabel->scaleId = Traveler::ScaleId::SCALE_4;
		addChild(pScaleLabel);

		left = 40.f;
		// top = 99.f;

		pRootNoteLabel = createWidget<RootNoteLabel>(mm2px(Vec(left, top)));
		pRootNoteLabel->setModule(module); 
		pRootNoteLabel->scaleId = Traveler::ScaleId::SCALE_1;
		addChild(pRootNoteLabel);

		pRootNoteLabel = createWidget<RootNoteLabel>(mm2px(Vec(left, top + (8.5f * 1.f))));
		pRootNoteLabel->setModule(module); 
		pRootNoteLabel->scaleId = Traveler::ScaleId::SCALE_2;
		addChild(pRootNoteLabel);

		pRootNoteLabel = createWidget<RootNoteLabel>(mm2px(Vec(left, top + (8.5f * 2.f))));
		pRootNoteLabel->setModule(module); 
		pRootNoteLabel->scaleId = Traveler::ScaleId::SCALE_3;
		addChild(pRootNoteLabel);

		pRootNoteLabel = createWidget<RootNoteLabel>(mm2px(Vec(left, top + (8.5f * 3.f))));
		pRootNoteLabel->setModule(module); 
		pRootNoteLabel->scaleId = Traveler::ScaleId::SCALE_4;
		addChild(pRootNoteLabel);

		// BPM label
		left = 15.f;
		top = 16.f;

		BpmLabel * pBpmLabel = createWidget<BpmLabel>(mm2px(Vec(left, top)));
		pBpmLabel->setModule(module); 
		addChild(pBpmLabel);

		RoverAreaWidget * pRoverAreaWidget = createRoverAreaWidget(mm2px(Vec(72.627 + 29., 17.077)), mm2px(Vec(94.627, 94.627)));
		pRoverAreaWidget->setModule(module);
		pRoverAreaWidget->setIconPalette(iconPalette);
		addChild(pRoverAreaWidget);

		left = 66.f;
		top = 12.f;
		float cell_size = 8.f;
		float col_2 = left + cell_size;
		float col_3 = left + cell_size * 2;
		float row_2 = top  + cell_size;
		float row_3 = top  + cell_size * 2;
		
		addRoverIcon(ROVER_NORTHWEST, SPEED_NORMAL, mm2px(Vec(left,  top)), pRoverAreaWidget->roverImageTable);
		addRoverIcon(ROVER_NORTH,     SPEED_NORMAL, mm2px(Vec(col_2, top)), pRoverAreaWidget->roverImageTable);
		addRoverIcon(ROVER_NORTHEAST, SPEED_NORMAL, mm2px(Vec(col_3, top)), pRoverAreaWidget->roverImageTable);
		addRoverIcon(ROVER_WEST,      SPEED_NORMAL, mm2px(Vec(left,  row_2)), pRoverAreaWidget->roverImageTable);
		addRoverIcon(ROVER_EAST,      SPEED_NORMAL, mm2px(Vec(col_3, row_2)), pRoverAreaWidget->roverImageTable);
		addRoverIcon(ROVER_SOUTHWEST, SPEED_NORMAL, mm2px(Vec(left,  row_3)), pRoverAreaWidget->roverImageTable);
		addRoverIcon(ROVER_SOUTH,     SPEED_NORMAL, mm2px(Vec(col_2, row_3)), pRoverAreaWidget->roverImageTable);
		addRoverIcon(ROVER_SOUTHEAST, SPEED_NORMAL, mm2px(Vec(col_3, row_3)), pRoverAreaWidget->roverImageTable);

		top = 40.f;
		left = 66.f; // 69.f;
		for (int k = 0; k < 14; k++) {
			if ((k & 1) == 0) { // even 
				addObstacleIcon((ObstacleType)k, mm2px(Vec(left,  top)), pRoverAreaWidget->obstacleImageTable);
			} else {
				addObstacleIcon((ObstacleType)k, mm2px(Vec(left + cell_size,  top)), pRoverAreaWidget->obstacleImageTable);
				top += cell_size;
			}
		}		

		left = 66.f;
		for (int k = 14; k < NUM_OBSTACLE_TYPES; k += 3) {
			addObstacleIcon((ObstacleType)(k+0), mm2px(Vec(left,  top)), pRoverAreaWidget->obstacleImageTable);
			addObstacleIcon((ObstacleType)(k+1), mm2px(Vec(col_2,  top)), pRoverAreaWidget->obstacleImageTable);
			addObstacleIcon((ObstacleType)(k+2), mm2px(Vec(col_3,  top)), pRoverAreaWidget->obstacleImageTable);
			top += cell_size;
			top += cell_size * 0.5;
		}		

		iconPalette.refreshSelected();
	}

	void appendContextMenu(Menu *menu) override
	{
		Traveler *module = dynamic_cast<Traveler*>(this->module);
		assert(module);

        menu->addChild(new MenuSeparator());
		// menu->addChild(createMenuLabel("Polyphony"));

		PolyphonyMenu *polySubMenu = createMenuItem<PolyphonyMenu>("Polyphony", RIGHT_ARROW);
		polySubMenu->module = module;
		menu->addChild(polySubMenu);

        menu->addChild(new MenuSeparator());
		menu->addChild(createMenuLabel("Scale 1"));

		SelectScaleFile<Traveler> *menu_item = new SelectScaleFile<Traveler>();
		menu_item->module = module;
		menu_item->scaleId = Traveler::SCALE_1;
		menu_item->text = module->scales[Traveler::SCALE_1].filepath;
		menu->addChild(menu_item);

		menu->addChild(createMenuLabel("Scale 2"));
		menu_item = new SelectScaleFile<Traveler>();
		menu_item->module = module;
		menu_item->scaleId = Traveler::SCALE_2;
		menu_item->text = module->scales[Traveler::SCALE_2].filepath;
		menu->addChild(menu_item);

		menu->addChild(createMenuLabel("Scale 3"));
		menu_item = new SelectScaleFile<Traveler>();
		menu_item->module = module;
		menu_item->scaleId = Traveler::SCALE_3;
		menu_item->text = module->scales[Traveler::SCALE_3].filepath;
		menu->addChild(menu_item);

		menu->addChild(createMenuLabel("Scale 4"));
		menu_item = new SelectScaleFile<Traveler>();
		menu_item->module = module;
		menu_item->scaleId = Traveler::SCALE_4;
		menu_item->text = module->scales[Traveler::SCALE_4].filepath;
		menu->addChild(menu_item);
	}

// look here  https://community.vcvrack.com/t/framebufferwidget-question/3041
// Use FrameBufferWidget
//Follow pattern for SvgKnobs 
// See https://github.com/VCVRack/Rack/blob/a5fc58910bcdfdd6babd77bafad7bbe142ac939a/src/app/SvgKnob.cpp
// set the framebuffer.image to the image handle 

	void addRoverIcon(RoverType roverType, SpeedType speedType, Vec pos, std::shared_ptr<Svg> iconSvgTable[NUM_SPEED_TYPES][NUM_ROVER_TYPES]) {
		RoverFieldCellIcon * pIcon = createWidget<RoverFieldCellIcon>(pos);
		pIcon->setIconPalette(iconPalette);
		pIcon->setSvg(iconSvgTable[int(speedType)][int(roverType)]);
		pIcon->setRoverType(roverType, speedType);
		addChild(pIcon);

		if (speedType == SPEED_NORMAL) {
			iconPalette.roverIcons[ int(roverType) ] = pIcon;
		}
	}

	void addObstacleIcon(ObstacleType obstacleType, Vec pos, std::shared_ptr<Svg> iconSvgTable[]) {
		RoverFieldCellIcon * pIcon = createWidget<RoverFieldCellIcon>(pos);
		pIcon->setIconPalette(iconPalette);
		pIcon->setSvg(iconSvgTable[int(obstacleType)]);
		pIcon->setObstacleType(obstacleType);
		addChild(pIcon);

		iconPalette.obstacleIcons[ int(obstacleType) ] = pIcon;
	}

};

Model* modelTraveler = createModel<Traveler, TravelerWidget>("Traveler");