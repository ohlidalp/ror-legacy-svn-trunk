#pragma once
#ifndef AITRAFFIC_VEHICLE_H
#define AITRAFFIC_VEHICLE_H

#include "Ogre.h"
#include "OpenSteer/AbstractVehicle.h"
#include "OpenSteer/SteerLibrary.h"

using namespace OpenSteer;

// SimpleVehicle_1 adds concrete LocalSpace methods to AbstractVehicle
typedef OpenSteer::LocalSpaceMixin<AbstractVehicle> SimpleVehicle_1;

// SimpleVehicle_3 adds concrete steering methods to SimpleVehicle_2
typedef OpenSteer::SteerLibraryMixin<SimpleVehicle_1> SimpleVehicle_2;

// SimpleVehicle adds concrete vehicle methods to SimpleVehicle_2

class AITraffic_Vehicle: public SimpleVehicle_2
{

public:

	AITraffic_Vehicle(); 

	~AITraffic_Vehicle();

	// Following methods derive from pure virtuals in AbstractVehicle.
	// AITraffic_Vehicle is basically a straight copy of the SimpleVehicle class implementation 
	// excluding OpenSteer Annotation functionality.

    // reset vehicle state
    void reset(void)
    {
        // reset LocalSpace state
        resetLocalSpace ();

        // reset SteerLibraryMixin state
        SimpleVehicle_2::reset ();

        setMass (1);          // mass (defaults to 1 so acceleration=force)
        setSpeed (0);         // speed along Forward direction.

        setRadius (0.5f);     // size of bounding sphere

        setMaxForce (0.1f);   // steering force is clipped to this magnitude
        setMaxSpeed (1.0f);   // velocity is clipped to this magnitude

        // reset bookkeeping to do running averages of these quanities
        resetSmoothedPosition ();
        resetSmoothedCurvature ();
        resetSmoothedAcceleration ();
    }

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

    // set a random "2D" heading: set local Up to global Y, then effectively
    // rotate about it by a random angle (pick random forward, derive side).
    void randomizeHeadingOnXZPlane (void)
    {
        setUp (Vec3::up);
        setForward (RandomUnitVectorOnXZPlane ());
        setSide (localRotateForwardToSide (forward()));
    }

	Ogre::Vector3 getPosition();
	void setPosition(Ogre::Vector3 newPos);

	void update(const float deltat, const float totalTime);

	Ogre::Vector3 getDirection();

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
		float _smoothedCurvature;

		OpenSteer::Vec3 _lastForward;
        OpenSteer::Vec3 _lastPosition;
        OpenSteer::Vec3 _smoothedPosition;
        OpenSteer::Vec3 _smoothedAcceleration;

        // measure path curvature (1/turning-radius), maintain smoothed version
        void measurePathCurvature (const float elapsedTime);

};

#endif
