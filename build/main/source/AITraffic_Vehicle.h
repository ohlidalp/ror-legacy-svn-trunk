#ifdef OPENSTEER
#pragma once
#ifndef AITRAFFIC_VEHICLE_H
#define AITRAFFIC_VEHICLE_H

#include "Ogre.h"
#include "OpenSteer/AbstractVehicle.h"
#include "OpenSteer/LocalSpace.h"
#include "OpenSteer/SteerLibrary.h"
#include "OpenSteer/Utilities.h"
#include "OpenSteer/Pathway.h"
#include "AITraffic_Route.h"
#include "AITraffic_TerrainMap.h"

using namespace OpenSteer;

// SimpleVehicle_1 adds concrete LocalSpace methods to AbstractVehicle
typedef OpenSteer::LocalSpaceMixin<AbstractVehicle> SimpleVehicle_1IMI;

// SimpleVehicle_3 adds concrete steering methods to SimpleVehicle_2
typedef OpenSteer::SteerLibraryMixin<SimpleVehicle_1IMI> SimpleVehicle_2IMI;

// SimpleVehicle adds concrete vehicle methods to SimpleVehicle_2

//class AITraffic_Vehicle: public SimpleVehicle_2IMI
class AITraffic_Vehicle: public SimpleVehicle_2IMI
{

	public:

		AITraffic_Vehicle(); 
		~AITraffic_Vehicle();

		/* from opensteer map demo */
		void reset (void);
		void update (const float currentTime, const float elapsedTime);
		void adjustVehicleRadiusForSpeed (void);
		void collectReliabilityStatistics (const float currentTime, const float elapsedTime);
		OpenSteer::Vec3 hintForObstacleAvoidance (void);
		OpenSteer::Vec3 steerToAvoidObstaclesOnMap (const float minTimeToCollision,const AITraffic_TerrainMap& map);
		OpenSteer::Vec3 steerToAvoidObstaclesOnMap (const float minTimeToCollision, const AITraffic_TerrainMap& map, const OpenSteer::Vec3& steerHint);
		void annotateAvoidObstaclesOnMap (const OpenSteer::Vec3& scanOrigin, int scanIndex, const OpenSteer::Vec3& scanStep);
		void annotationNoteOAClauseName (const char* clauseName);
		void annotationHintWasTaken (void);
		float scanObstacleMap (	const OpenSteer::Vec3& start,
								const OpenSteer::Vec3& center,
								const float arcAngle,
								const int segments,
								const float endRadiusChange,
								const OpenSteer::Vec3& beforeColor,
								const OpenSteer::Vec3& afterColor,
								OpenSteer::Vec3& returnObstaclePosition);
		bool detectImminentCollision (void);
		OpenSteer::Vec3 predictFuturePosition (const float predictionTime);
		float arcLengthLimit (const float length, const float limit);
		OpenSteer::Vec3 steerToFollowPath (const int direction, const float predictionTime, AITraffic_Route &path);
		OpenSteer::Vec3 steerToFollowPathLinear (const int direction, const float predictionTime, AITraffic_Route &path);
		OpenSteer::Vec3 steerToFollowPathCurve (const int direction, const float predictionTime, AITraffic_Route &path);
		void perFrameAnnotation (void);
		AITraffic_Route* makePath (void);
		AITraffic_TerrainMap* makeMap(void);
		bool handleExitFromMap (void);
		float relativeSpeed (void) const;
		float wingSlope (void);
		void resetStuckCycleDetection (void);
		bool weAreGoingInCircles (void);
		float lookAheadTimeOA (void) const;
		float lookAheadTimePF (void) const;
		float combinedLookAheadTime (float minTime, float minDistance) const;
		bool bodyInsidePath (void);
		OpenSteer::Vec3 convertAbsoluteToIncrementalSteering (const OpenSteer::Vec3& absolute,
                                               const float elapsedTime);
		OpenSteer::Vec3 convertLinearToCurvedSpaceGlobal (const OpenSteer::Vec3& linear);
		float minimumTurningRadius ();
		OpenSteer::Vec3 adjustSteeringForMinimumTurningRadius (const OpenSteer::Vec3& steering);
		float nonZeroCurvatureQQQ (void);
		float maxSpeedForCurvature ();
		OpenSteer::Vec3 steerTowardHeading (const OpenSteer::Vec3& desiredGlobalHeading);

		//--Direct transport from OS:SimpleVehicle

        // get/set mass
        float mass (void) const {return _mass;}
        float setMass (float m) {return _mass = m;}

        // get velocity of vehicle
        Vec3 velocity (void) const {return forward() * _speed;}

        // get/set speed of vehicle  (may be faster than taking mag of velocity)
        float speed (void) const {return _speed;}
        float setSpeed (float s) {return _speed = s;}

        // size of bounding sphere, for obstacle avoidance, etc.
        float radius (void) const {return _radius;}
        float setRadius (float m) {return _radius = m;}

        // get/set maxForce
        float maxForce (void) const {return _maxForce;}
        float setMaxForce (float mf) {return _maxForce = mf;}

        // get/set maxSpeed
        float maxSpeed (void) const {return _maxSpeed;}
        float setMaxSpeed (float ms) {return _maxSpeed = ms;}


        // apply a given steering force to our momentum,
        // adjusting our orientation to maintain velocity-alignment.
        void applySteeringForce (const Vec3& force, const float deltaTime);

        // the default version: keep FORWARD parallel to velocity, change
        // UP as little as possible.
        virtual void regenerateLocalSpace (const Vec3& newVelocity,
                                           const float elapsedTime);

        // alternate version: keep FORWARD parallel to velocity, adjust UP
        // according to a no-basis-in-reality "banking" behavior, something
        // like what birds and airplanes do.  (XXX experimental cwr 6-5-03)
        void regenerateLocalSpaceForBanking (const Vec3& newVelocity,
                                             const float elapsedTime);

        // adjust the steering force passed to applySteeringForce.
        // allows a specific vehicle class to redefine this adjustment.
        // default is to disallow backward-facing steering at low speed.
        // xxx experimental 8-20-02
        virtual Vec3 adjustRawSteeringForce (const Vec3& force,
                                             const float deltaTime);

        // apply a given braking force (for a given dt) to our momentum.
        // xxx experimental 9-6-02
        void applyBrakingForce (const float rate, const float deltaTime);

        // predict position of this vehicle at some time in the future
        // (assumes velocity remains constant)
        Vec3 predictFuturePosition (const float predictionTime) const;

        // get instantaneous curvature (since last update)
        float curvature (void) {return _curvature;}

        // get/reset smoothedCurvature, smoothedAcceleration and smoothedPosition
        float smoothedCurvature (void) {return _smoothedCurvature;}
        float resetSmoothedCurvature (float value = 0)
        {
            _lastForward = Vec3::zero;
            _lastPosition = Vec3::zero;
            return _smoothedCurvature = _curvature = value;
        }
        Vec3 smoothedAcceleration (void) {return _smoothedAcceleration;}
        Vec3 resetSmoothedAcceleration (const Vec3& value = Vec3::zero)
        {
            return _smoothedAcceleration = value;
        }
        Vec3 smoothedPosition (void) {return _smoothedPosition;}
        Vec3 resetSmoothedPosition (const Vec3& value = Vec3::zero)
        {
            return _smoothedPosition = value;
        }

		Ogre::Vector3 getPosition();
		Ogre::Quaternion getOrientation();
		void setPosition(Ogre::Vector3 newPos);

        // give each vehicle a unique number
        int serialNumber;
        static int serialNumberCounter;

        // draw lines from vehicle's position showing its velocity and acceleration
        void annotationVelocityAcceleration (float maxLengthA, float maxLengthV);
        void annotationVelocityAcceleration (float maxLength)
            {annotationVelocityAcceleration (maxLength, maxLength);}
        void annotationVelocityAcceleration (void)
            {annotationVelocityAcceleration (3, 3);}

        // set a random "2D" heading: set local Up to global Y, then effectively
        // rotate about it by a random angle (pick random forward, derive side).
        void randomizeHeadingOnXZPlane (void)
        {
            setUp (Vec3::up);
            setForward (RandomUnitVectorOnXZPlane ());
            setSide (localRotateForwardToSide (forward()));
        }

	
	//--Variables

		AITraffic_TerrainMap* map;					// map of obstacles
		AITraffic_Route* path;						// route for path following (waypoints and legs)
		int pathFollowDirection;			// follow the path "upstream or downstream" (+1/-1)
		float baseLookAheadTime;			// master look ahead (prediction) time
    
		float halfWidth;					// vehicle dimentions in meters
		float halfLength;
    
		bool collisionDetected;				// keep track of failure rate (when vehicle is on top of obstacle)
		bool collisionLastTime;
		float timeOfLastCollision;
		float sumOfCollisionFreeTimes;
		int countOfCollisionFreeTimes;

		float totalDistance;				// keep track of average speed
		float totalTime;
    
		float pathFollowTime;				// keep track of path following failure rate
		float pathFollowOffTime;			// (these are probably obsolete now, replaced by stuckOffPathCount)

		bool dtZero;						// take note when current dt is zero (as in paused) for stat counters

		OpenSteer::Vec3 annotateAvoid;					// state saved for annotation
		bool wingDrawFlagL, wingDrawFlagR;

		bool stuck;							// first pass at detecting "stuck" state
		int stuckCount;
		int stuckCycleCount;
		int stuckOffPathCount;

		OpenSteer::Vec3 qqqLastNearestObstacle;

		int lapsStarted;
		int lapsFinished;
	
		bool QQQoaJustScraping;				// temporary global QQQoaJustScraping, replace this with a cleaner mechanism

		int hintGivenCount;
		int hintTakenCount;
    
		OpenSteer::Vec3 currentSteering;				// for "curvature-based incremental steering" -- contains the current
										// steering into which new incremental steering is blended

		bool curvedSteering;				// use curved prediction and incremental steering:
		bool incrementalSteering;
    
		float savedNearestWR;		// save obstacle avoidance stats for annotation
		float savedNearestR;
		float savedNearestL;
		float savedNearestWL; 

		float annoteMaxRelSpeed;
		float annoteMaxRelSpeedCurve;
		float annoteMaxRelSpeedPath;

		int demoSelect;				// which of the three demo modes is selected

		float worldSize;				// size of the world (the map actually)
		float worldDiag;

    private:

        float _mass;       // mass (defaults to unity so acceleration=force)

        float _radius;     // size of bounding sphere, for obstacle avoidance, etc.

        float _speed;      // speed along Forward direction.  Because local space
                           // is velocity-aligned, velocity = Forward * Speed

        float _maxForce;   // the maximum steering force this vehicle can apply
                           // (steering force is clipped to this magnitude)

        float _maxSpeed;   // the maximum speed this vehicle is allowed to move
                           // (velocity is clipped to this magnitude)

        float _curvature;
        Vec3 _lastForward;
        Vec3 _lastPosition;
        Vec3 _smoothedPosition;
        float _smoothedCurvature;
        Vec3 _smoothedAcceleration;

        // measure path curvature (1/turning-radius), maintain smoothed version
        void measurePathCurvature (const float elapsedTime);


};

#endif
#endif //OPENSTEER
    