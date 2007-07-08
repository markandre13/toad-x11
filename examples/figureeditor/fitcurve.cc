// the stuff below is modified GPLed code from Xara LX

/*
================================XARAHEADERSTART===========================
 
               Xara LX, a vector drawing and manipulation program.
                    Copyright (C) 1993-2006 Xara Group Ltd.
       Copyright on certain contributions may be held in joint with their
              respective authors. See AUTHORS file for details.

LICENSE TO USE AND MODIFY SOFTWARE
----------------------------------

This file is part of Xara LX.

Xara LX is free software; you can redistribute it and/or modify it
under the terms of the GNU General Public License version 2 as published
by the Free Software Foundation.

Xara LX and its component source files are distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with Xara LX (see the file GPL in the root directory of the
distribution); if not, write to the Free Software Foundation, Inc., 51
Franklin St, Fifth Floor, Boston, MA  02110-1301 USA


ADDITIONAL RIGHTS
-----------------

Conditional upon your continuing compliance with the GNU General Public
License described above, Xara Group Ltd grants to you certain additional
rights. 

The additional rights are to use, modify, and distribute the software
together with the wxWidgets library, the wxXtra library, and the "CDraw"
library and any other such library that any version of Xara LX relased
by Xara Group Ltd requires in order to compile and execute, including
the static linking of that library to XaraLX. In the case of the
"CDraw" library, you may satisfy obligation under the GNU General Public
License to provide source code by providing a binary copy of the library
concerned and a copy of the license accompanying it.

Nothing in this section restricts any of the rights you have under
the GNU General Public License.


SCOPE OF LICENSE
----------------

This license applies to this program (XaraLX) and its constituent source
files only, and does not necessarily apply to other Xara products which may
in part share the same code base, and are subject to their own licensing
terms.

This license does not apply to files in the wxXtra directory, which
are built into a separate library, and are subject to the wxWindows
license contained within that directory in the file "WXXTRA-LICENSE".

This license does not apply to the binary libraries (if any) within
the "libs" directory, which are subject to a separate license contained
within that directory in the file "LIBS-LICENSE".


ARRANGEMENTS FOR CONTRIBUTION OF MODIFICATIONS
----------------------------------------------

Subject to the terms of the GNU Public License (see above), you are
free to do whatever you like with your modifications. However, you may
(at your option) wish contribute them to Xara's source tree. You can
find details of how to do this at:
  http://www.xaraxtreme.org/developers/

Prior to contributing your modifications, you will need to complete our
contributor agreement. This can be found at:
  http://www.xaraxtreme.org/developers/contribute/

Please note that Xara will not accept modifications which modify any of
the text between the start and end of this header (marked
XARAHEADERSTART and XARAHEADEREND).


MARKS
-----

Xara, Xara LX, Xara X, Xara X/Xtreme, Xara Xtreme, the Xtreme and Xara
designs are registered or unregistered trademarks, design-marks, and/or
service marks of Xara Group Ltd. All rights in these marks are reserved.


      Xara Group Ltd, Gaddesden Place, Hemel Hempstead, HP2 6EX, UK.
                        http://www.xara.com/

=================================XARAHEADEREND============================
*/

#include <toad/toad.hh>
#include <toad/figure.hh>
#include <cmath>

using namespace toad;

// the stuff below is GPLed code from Xara:

#define STEP_SIZE       6
#define ERROR_STEP      6

typedef TPoint DocCoord;


struct FitPoint {
  double x, y;

	FitPoint() {}
	FitPoint(double cx, double cy) { x=cx; y=cy;}
	FitPoint(DocCoord coord) { x=coord.x; y=coord.y; }
	FitPoint(const FitPoint& Other) {x=Other.x; y=Other.y;}
	~FitPoint() {}

	void operator=(const FitPoint& Other) { x=Other.x; y=Other.y; }
	void operator=(const DocCoord& Other) { x=Other.x; y=Other.y; }

  FitPoint operator-() const;
  FitPoint operator*(double Factor) const;

	friend FitPoint operator + (const FitPoint& Point1, const FitPoint& Point2);
	friend FitPoint operator - (const FitPoint& Point1, const FitPoint& Point2);

	// General geometry functions
	double squaredLength() const { return (x*x + y*y); }
	double length() const { return sqrt(x*x + y*y); }
	double dot(const FitPoint& Other)	const { return (Other.x*x + Other.y*y); }

	FitPoint setLength(double NewLen) const;
};

FitPoint operator+(const FitPoint& Point1, const FitPoint& Point2)
{
	FitPoint Result;

	// Add the two vector together
	Result.x = Point1.x + Point2.x;
	Result.y = Point1.y + Point2.y;

	// return the result
	return Result;
}

FitPoint operator-(const FitPoint& Point1, const FitPoint& Point2)
{
	FitPoint Result;

	// Subtract the two vector from each other
	Result.x = Point1.x - Point2.x;
	Result.y = Point1.y - Point2.y;

	// return the result
	return Result;
}


FitPoint
FitPoint::operator-() const {
	FitPoint Result;

	// negate the vector
	Result.x = -x;
	Result.y = -y;

	// and return it
	return Result;
}

FitPoint
FitPoint::operator*(double Factor) const {
	FitPoint Result;

	// Scale the vector by the factor
	Result.x = x*Factor;
	Result.y = y*Factor;

	// and return it
	return Result;
}

FitPoint
FitPoint::setLength(double NewLen) const
{
	FitPoint Result(x, y);

	double Len = length();
	if (Len != 0.0)	{
		Len = NewLen/Len ;
		Result.x *= Len;
		Result.y *= Len;
	}

	return Result;
}

struct CurveFitObject {
  CurveFitObject() {
    PathArray = 0;
    Distances = 0;
    Error = 50.0 * 960.0;
  }
  ~CurveFitObject() {
    if (PathArray) delete[] PathArray;
    if (Distances) delete[] Distances;
  }

  DocCoord *PathArray;
  int TotalCoords;
  
  TPolygon* fig;

	// An array that holds the distance of each point from the start of the path
	int* Distances;

	// The accuracy of the required fit. The larger this number, the smoother the fit.
	// Values of about 27,000,000 give smooth curves in millipoints at 100% zoom factor
	double		Error;


  bool initialise(const TPolygon &in, TPolygon *out);
  void fitCurve();
  
  FitPoint bezierPoint( FitPoint* Bez, double u) const;
  double calcMaxError(int FirstPoint, int LastPoint, FitPoint* Bezier, int* SplitPoint) const;
  FitPoint leftTangent(int Start) const;
  FitPoint rightTangent(int End) const;
  FitPoint centreTangent(int Centre) const;

  void fitCubic(int FirstPoint, int LastPoint,
                FitPoint Tangent1, FitPoint Tangent2,
                bool IsStartCusp = true, bool IsEndCusp = true);


	// Functions to put curve elements into the path
	void insertBezier(FitPoint* Bezier, bool, bool);
	void insertLine(const DocCoord& Start, const DocCoord& End, FitPoint Tangent1, FitPoint Tangent2, bool, bool);
	void insertStraightLine(const DocCoord& End);
 
  void generateBezier(int FirstPoint, int LastPoint,
                      FitPoint Tangent1, FitPoint Tangent2, 
                      FitPoint* Bezier);
};

// Functions to evaluate various components of the bezier function
inline double Bezier0(double u) { double t=1.0-u; return (t*t*t); }
inline double Bezier1(double u) { double t=1.0-u; return (3*u*t*t); }
inline double Bezier2(double u) { double t=1.0-u; return (3*u*u*t); }
inline double Bezier3(double u) { return (u*u*u); }

bool
CurveFitObject::initialise(const TPolygon &polygon, TPolygon *out)
{
  fig = out;

	// Here we must try and get some memory for the path array
	if (polygon.size()<2)
	  return false;

	PathArray = new DocCoord[polygon.size()];
	if (PathArray==NULL)
		return false;

	int NumPoints = TotalCoords = polygon.size();
	for(int i=0; i<TotalCoords; ++i) {
	  PathArray[i].x = polygon[i].x;
	  PathArray[i].y = polygon[i].y;
	}
#if 0
	// copy the data out of the path and into the array. Only copy points of interest
	CopyPath->FindStartOfPath();
	DocCoord* Coords = CopyPath->GetCoordArray();

	// Deal with the flags - We have to look for Spare1 being true, meaning that the previous
	// Lineto was meant to stay as a straight line
	PathFlags* Flags = CopyPath->GetFlagArray();
	TotalStraightLines = 0;
	for (int i=0; i<NumPoints; i++)	{
		if (Flags[i].Spare1==TRUE)
			TotalStraightLines++;
	}

	// Get some memory to store the positions of the line breaks in
	if (TotalStraightLines>0)	{
		LineArray = new int[TotalStraightLines];
		if (LineArray==NULL) {
			delete PathArray;
			PathArray = NULL;
			return FALSE;
		}
	}

	// copy the MoveTo out of the path and into our array
	PathArray[0].x = Coords[0].x;
	PathArray[0].y = Coords[0].y;

	int IncludePoint = 1;
	int StraightLinePos = 0;
	for (int i=1; i<NumPoints; i++)	{
		// If this is one of the straight line bits, then make a note of its position
		if (Flags[i].Spare1==TRUE) {
			LineArray[StraightLinePos] = IncludePoint;
			StraightLinePos++;
		}

		// Check to see if this coordinate is really needed (last point is always needed)
		if ((Coords[i].x != PathArray[IncludePoint-1].x) || (Coords[i].y != PathArray[IncludePoint-1].y) &&
			(i!=NumPoints-1))
		{
			// This point is not the same as the one before, so add it into the array
			PathArray[IncludePoint].x = Coords[i].x;
			PathArray[IncludePoint].y = Coords[i].y;

			IncludePoint++;
		}
	}

	// Add the last point in the track data to the path array if it is not already there
	if ((PathArray[IncludePoint-1].x != Coords[NumPoints-1].x) ||
		(PathArray[IncludePoint-1].y != Coords[NumPoints-1].y))
	{
		PathArray[IncludePoint].x = Coords[NumPoints-1].x;
		PathArray[IncludePoint].y = Coords[NumPoints-1].y;
		IncludePoint++;
	}

	// increment the Include count (as we have just added a point to the array) and then
	// set the true number of points properly.
	NumPoints = IncludePoint;

	if (NumPoints<2) {
		delete PathArray;
		delete LineArray;
		PathArray = NULL;
		LineArray = NULL;
		return false;
	}
#endif
	// Build an array of the distances along the path
	Distances = new int[NumPoints];
	if (Distances==NULL) {
		delete PathArray;
		PathArray = NULL;
		return false;
	}

	Distances[0] = 0;
	int dx, dy, min;
	for (int i=1; i<NumPoints; i++)	{
		// This is doing an approximation to a Square Root
		// It is about 250 times faster on a machine without FPU

		// find the difference between the last 2 points
		dx = fabs(PathArray[i].x - PathArray[i-1].x);
		dy = fabs(PathArray[i].y - PathArray[i-1].y);

		// Find out half the smallest of dx and dy
		if (dx>dy)
			min = dy>>1;
		else
			min = dx>>1;

		Distances[i] = Distances[i-1] + dx + dy - min;
	}

	// Now we can delete the Path Data in the path we are to put the smoothed path in
#if 1
  fig->clear();
  fig->addPoint(PathArray[0].x, PathArray[0].y);
#else
	LongPath->ClearPath();
	LongPath->FindStartOfPath();
	LongPath->InsertMoveTo(PathArray[0]);
#endif
	// Store the total number of coords in the array for future reference
	TotalCoords = NumPoints;

	return true;
}

void
CurveFitObject::fitCurve()
{
  FitPoint Tangent1, Tangent2;
  double Angle1, Angle2;
  int Start = 0;
	int StraightLinePos = 0;

	for(int i=1; i<TotalCoords-1; i++) {
    // Go find the angle between a group of points
		Angle1 = atan2((double)PathArray[i].y-PathArray[i-1].y, (double)PathArray[i].x-PathArray[i-1].x);
		Angle2 = atan2((double)PathArray[i+1].y-PathArray[i].y, (double)PathArray[i+1].x-PathArray[i].x);
		
		// Get them in a sensible range
		if (Angle1 < -M_PI)	Angle1 += 2*M_PI;
		if (Angle1 > M_PI)	Angle1 -= 2*M_PI;
		if (Angle2 < -M_PI)	Angle2 += 2*M_PI;
		if (Angle2 > M_PI)	Angle2 -= 2*M_PI;
		
		// See if this point is a cusp in the curve
		if ((fabs(Angle2-Angle1) > (M_PI/2)) && (fabs(Angle2-Angle1) <= M_PI)) {
			// calculate the tangents off the end of the path
			Tangent1 = leftTangent(Start);
			Tangent2 = rightTangent(i);

		  // and do a load of maths that will hopefully fit a nice curve on it
			fitCubic(Start, i, Tangent1, Tangent2);
			Start = i;
		}
	}

	int End = TotalCoords-1;
  // Just have to fit a curve from the last cusp to the end of the path
	Tangent1 = leftTangent(Start);
	Tangent2 = rightTangent(End);
    
	// and do a load of maths that will hopefully fit a nice curve on it
	fitCubic(Start, End, Tangent1, Tangent2);
}

FitPoint
CurveFitObject::bezierPoint( FitPoint* Bez, double u) const
{
	double OneMinus = 1.0-u;
	double uSquared = u*u;
	double OneMinusSquared = OneMinus*OneMinus;

	FitPoint Coord;
	Coord = Bez[0]*(OneMinusSquared*OneMinus);
	Coord = Coord + Bez[1]*(3.0*u*OneMinusSquared);
	Coord = Coord + Bez[2]*(3.0*uSquared*OneMinus);
	Coord = Coord + Bez[3]*(uSquared*u);

	return Coord;
}

double
CurveFitObject::calcMaxError(int FirstPoint, int LastPoint, FitPoint* Bezier, int* SplitPoint) const
{
	double		Distance;
	double		MaxDist = 0.0;
	double 		RTotalLength = 1.0/(Distances[LastPoint] - Distances[FirstPoint]);
	FitPoint	Point;

	// Start out by putting the split point in the middle of the curve segment
	int NumPoints = LastPoint - FirstPoint + 1;
	*SplitPoint = NumPoints / 2;
	int step = (NumPoints+ERROR_STEP) / ERROR_STEP;
	
	// Loop though the points, visiting a fixed number of them
	for (int i=FirstPoint+1; i<LastPoint; i+=step) {
		// Calculate the offset at this point
		double Offset = Distances[i] - Distances[FirstPoint];
		Offset *= RTotalLength;

		// Calculate where the curve actually is and find the distance from where we want it
		FitPoint Coord = PathArray[i];
		Point = bezierPoint(Bezier, Offset);
		Distance = (Point - Coord).squaredLength();
		if ( Distance >= MaxDist)	{
			MaxDist = Distance;
			*SplitPoint = i;
		}
	}
	
	return MaxDist;
}

FitPoint
CurveFitObject::leftTangent(int Start) const
{
	FitPoint Tangent;

	// check for empty point array
	if (TotalCoords == 0)	{
		Tangent.x = 1;
		Tangent.y = 0;
		return Tangent;
	}
	
	// check for start outside the array
	if ((Start >= TotalCoords) || (Start < 0))
		Start = TotalCoords / 2;

	// Find out which point to look to
	int Forward = Start+2;
	if (Forward > TotalCoords-1)
		Forward = TotalCoords-1;

	// Calc the tangent from the left of the curve segment
	Tangent.x = PathArray[Forward].x - PathArray[Start].x;
	Tangent.y = PathArray[Forward].y - PathArray[Start].y;

	// Make sure that is not of zero length
	if ((Tangent.x==0) && (Tangent.y==0))	{
		Tangent.x = 1;
	}
	
	return Tangent;
}


FitPoint 
CurveFitObject::rightTangent(int End) const
{
	FitPoint Tangent;

	// Find out which point to look to
	int Backward = End-2;
	if (Backward<0)
		Backward = 0;

	Tangent.x = PathArray[Backward].x - PathArray[End].x;
	Tangent.y = PathArray[Backward].y - PathArray[End].y;

	// Make sure that is not of zero length
	if ((Tangent.x==0) && (Tangent.y==0))	{
		Tangent.x = -1;
	}

	return Tangent;
}

FitPoint
CurveFitObject::centreTangent(int Centre) const
{
	DocCoord Left, Right;
	FitPoint CentreTang;

	// check for empty point array
	if (TotalCoords == 0)	{
		CentreTang.x = 1;
		CentreTang.y = 0;

		return CentreTang;
	}

	// check for centre outside the array
	if ((Centre >= TotalCoords) || (Centre < 0))
		Centre = TotalCoords / 2;

	// Find out which point to look to
	int Forward = Centre+2;
	if (Forward > TotalCoords-1)
		Forward = TotalCoords-1;

	// Find out which point to look to
	int Backward = Centre-2;
	if (Backward < 0)
		Backward = 0;

	// Calc right tangent
	Left.x = PathArray[Backward].x - PathArray[Centre].x;
	Left.y = PathArray[Backward].y - PathArray[Centre].y;

	// Calc left tangent
	Right.x = PathArray[Centre].x - PathArray[Forward].x;
	Right.y = PathArray[Centre].y - PathArray[Forward].y;

	// Average to get the centre tangent
	CentreTang.x = (Left.x + Right.x) / 2.0;
	CentreTang.y = (Left.y + Right.y) / 2.0;

	// Make sure that is not of zero length
	if ((CentreTang.x==0) && (CentreTang.y==0))	{
		CentreTang.x = 1;
	}

	// return it
	return CentreTang;
}


void
CurveFitObject::fitCubic(int FirstPoint, int LastPoint,
                         FitPoint Tangent1, FitPoint Tangent2,
                         bool IsStartCusp, bool IsEndCusp)
{
	// Will need space for a Bezier curve
	FitPoint Bezier[4];
	int NumPoints = LastPoint - FirstPoint + 1;

	// if this segment only has 2 points in it then do the special case
	if ( NumPoints == 2 )	{
		insertLine(PathArray[FirstPoint], PathArray[LastPoint], Tangent1, Tangent2, IsStartCusp, IsEndCusp);
		return;
	}
	
	// Due to a bug in the algorithm we also have to consider 3 points as a special case
	if ( NumPoints == 3 )	{
		int Distance = (Distances[LastPoint] - Distances[FirstPoint]) / 3;
		
		// store the end points
		Bezier[0] = PathArray[FirstPoint];
		Bezier[3] = PathArray[LastPoint];
		
		// calc the control points
		Bezier[1] = Bezier[0] + Tangent1.setLength(Distance);
		Bezier[2] = Bezier[3] + Tangent2.setLength(Distance);

		// add it to the path
		insertBezier(Bezier, IsStartCusp, IsEndCusp);
		return;
	}

	// Create a Bezier curve from the segemnt and see if it is a good fit
	int SplitPoint;
	generateBezier(FirstPoint, LastPoint, Tangent1, Tangent2, Bezier);
	double MaxError = calcMaxError(FirstPoint, LastPoint, Bezier, &SplitPoint);
	
	if (MaxError < Error)	{
		// The mapping was good, so output the curve segment
		insertBezier(Bezier, IsStartCusp, IsEndCusp);
		return;
	}
	
	// fitting failed -- split at max error point and fit recursively
	FitPoint CentTangent = centreTangent(SplitPoint);
	fitCubic(FirstPoint, SplitPoint, Tangent1, CentTangent, IsStartCusp, false);

	CentTangent = -CentTangent;
	fitCubic(SplitPoint, LastPoint, CentTangent, Tangent2, false, IsEndCusp);
}

void
CurveFitObject::insertBezier(FitPoint* Bezier, bool IsStartCusp, bool IsEndCusp)
{
	fig->addPoint((int)Bezier[1].x, (int)Bezier[1].y);
	fig->addPoint((int)Bezier[2].x, (int)Bezier[2].y);
	fig->addPoint((int)Bezier[3].x, (int)Bezier[3].y);
#if 0
	// Prepare some flags
	PathFlags Flags;
	Flags.IsSelected = FALSE;
	Flags.IsSmooth = FALSE;
	Flags.IsRotate = TRUE;

	// Add this Bezier curve into the path
	LongPath->InsertCurveTo(DocCoord( (int)Bezier[1].x, (int)Bezier[1].y),
                          DocCoord( (INT32)Bezier[2].x, (INT32)Bezier[2].y),
                          DocCoord( (INT32)Bezier[3].x, (INT32)Bezier[3].y), &Flags);

	// Deal with cusps
	if (IsStartCusp || IsEndCusp)	{
		// Go find out about the path
		PathFlags* AllFlags = LongPath->GetFlagArray();
		int NumCoords = LongPath->GetNumCoords();

		if (IsStartCusp) {
			// Patch up the flags of the bits near that start
			AllFlags[NumCoords-3].IsRotate = FALSE;
		}
	
		if (IsEndCusp) {
			// Patch up the flags of the bits near that end
			AllFlags[NumCoords-2].IsRotate = FALSE;
			AllFlags[NumCoords-1].IsRotate = FALSE;
		}
	}
#endif
}

void
CurveFitObject::insertLine(const DocCoord& Start, const DocCoord& End,
                           FitPoint Tangent1, FitPoint Tangent2,
                           bool IsStartCusp, bool IsEndCusp)
{
#if 0
	// Prepare some flags
	PathFlags Flags;
	Flags.IsSelected = FALSE;
	Flags.IsSmooth = FALSE;
	Flags.IsRotate = TRUE;
#endif
	// Find out a third of the distance between the two points
	FitPoint StartPos(Start);
	FitPoint EndPos(End);
	FitPoint DistanceVect = EndPos - StartPos;
	int Length = (int)DistanceVect.length() / 3;

	// Make the tangents the right length
	Tangent1 = Tangent1.setLength(Length);
	Tangent2 = Tangent2.setLength(Length);

	// Work out the position of the control points
  StartPos = StartPos + Tangent1;
	EndPos = EndPos + Tangent2;
	
	fig->addPoint((int)StartPos.x, (int)StartPos.y);
	fig->addPoint((int)EndPos.x,   (int)EndPos.y);
	fig->addPoint(End.x, End.y);
	
#if 0
	// Add the line to the curve
	LongPath->InsertCurveTo( DocCoord( (int)StartPos.x, (int)StartPos.y ), 
							 DocCoord( (int)EndPos.x, (int)EndPos.y ),
							 End, &Flags);	

	// Deal with cusps
	if (IsStartCusp || IsEndCusp)	{
		// Go find out about the path
		PathFlags* AllFlags = LongPath->GetFlagArray();
		INT32 NumCoords = LongPath->GetNumCoords();

		if (IsStartCusp) {
			// Patch up the flags of the bits near that start
			AllFlags[NumCoords-3].IsRotate = FALSE;
		}
	
		if (IsEndCusp) {
			// Patch up the flags of the bits near that end
			AllFlags[NumCoords-2].IsRotate = FALSE;
			AllFlags[NumCoords-1].IsRotate = FALSE;
		}
	}
#endif
}

void
CurveFitObject::generateBezier(int FirstPoint, int LastPoint,
                               FitPoint Tangent1, FitPoint Tangent2, 
                               FitPoint* Bezier)
{
	int NumPoints = LastPoint - FirstPoint + 1;
	
	// Build a matrix type of thing that contains the tangents scaled by the offsets
	FitPoint A[STEP_SIZE+1][2];			//	Vector2 (*A)[2] = new Vector2[NumPoints+1][2];
	double   Offsets[STEP_SIZE+1];

	int step = (NumPoints+STEP_SIZE) / STEP_SIZE;
	int i, pos = 0;

	// Chord length parameterisation
	const int DistToEnd = Distances[LastPoint] - Distances[FirstPoint];
	for (i=FirstPoint; i<LastPoint+1; i+=step) {
		Offsets[pos] = Distances[i] - Distances[FirstPoint];
		Offsets[pos] /= DistToEnd;

		// Fill the matrix A
		A[pos][0] = Tangent1.setLength( Bezier1(Offsets[pos]) );
		A[pos][1] = Tangent2.setLength( Bezier2(Offsets[pos]) );

		// Move to the next element in the path
		pos++;
	}

	// For a detailed description of the maths used here, please see Graphics Gems I
	// I have made some changes to the basic algorithm used there - the main one is
	// that this block of maths is only performed on a small selection of the points
	// in the data set, where-as the one in the book uses all the points
	double  C[2][2];
	double  X[2];
	
	C[0][0] = 0.0;
	C[0][1] = 0.0;
	C[1][0] = 0.0;
	C[1][1] = 0.0;
	X[0]    = 0.0;
	X[1]    = 0.0;
	
	FitPoint FirstCoord = PathArray[FirstPoint];
	FitPoint LastCoord  = PathArray[LastPoint];
	FitPoint ThisCoord, Combo;

	pos = 0;
	for (i=0; i<NumPoints; i+=step)	{
		C[0][0] += A[pos][0].squaredLength();
		C[0][1] += A[pos][0].dot(A[pos][1]);
		// Point C[1][0] is the same as C[0][1] and is set outside the loop below
		C[1][1] += A[pos][1].squaredLength();
		
		// Go ahead and build a vector based on the bezier functions
		ThisCoord = PathArray[FirstPoint+i];
		Combo = ThisCoord - ((FirstCoord * Bezier0(Offsets[pos]))
							+ (FirstCoord * Bezier1(Offsets[pos]))
							+ (LastCoord  * Bezier2(Offsets[pos]))
							+ (LastCoord  * Bezier3(Offsets[pos])));

		// Combine it with the other points
		X[0] += A[pos][0].dot( Combo );
		X[1] += A[pos][1].dot( Combo );

		pos++;
	}

	// This point in the matrix is the same, so we do not need to do it in the loop
	C[1][0] = C[0][1];
	
	// calc the determinants of C and X
	double det_C0_C1 = C[0][0] * C[1][1] - C[1][0] * C[0][1];
	double det_C0_X  = C[0][0] * X[1]    - C[0][1] * X[0];
	double det_X_C1  = X[0]    * C[1][1] - X[1]    * C[0][1];
	
	// finally, derive the length of the ideal tangents
	if (det_C0_C1 == 0.0)
		det_C0_C1 = (C[0][0] * C[1][1]) * 10e-12;	// oh err, whats it up to here then!
	
	double AlphaLeft  = det_X_C1 / det_C0_C1;
	double AlphaRight = det_C0_X / det_C0_C1;
	
	Bezier[0] = PathArray[FirstPoint];
	Bezier[3] = PathArray[LastPoint];

	// if alpha negative, the set the tangent length to a third of the distance between
	// the start and the end points of the curve segment	
	if ( AlphaLeft < 0.0 || AlphaRight < 0.0)	{
		int Distance = (Distances[LastPoint] - Distances[FirstPoint]) / 3;
		
		Bezier[1] = Bezier[0] + Tangent1.setLength(Distance);
		Bezier[2] = Bezier[3] + Tangent2.setLength(Distance);
	}	else {	
		Bezier[1] = Bezier[0] + Tangent1.setLength(AlphaLeft);
		Bezier[2] = Bezier[3] + Tangent2.setLength(AlphaRight);
	}
}

void
fitCurve(const TPolygon &in, TPolygon *out)
{
  CurveFitObject c;
  if (c.initialise(in, out)) {
    c.fitCurve();
  }
}
