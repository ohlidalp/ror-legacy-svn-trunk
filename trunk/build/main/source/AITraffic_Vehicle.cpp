#include "AITraffic_Vehicle.h"

#include "AITraffic_Vehicle.h"

AITraffic_Vehicle::AITraffic_Vehicle()
{
	reset();
}

AITraffic_Vehicle::~AITraffic_Vehicle()
{

}

// ----------------------------------------------------------------------------
// adjust the steering force passed to applySteeringForce.
//
// allows a specific vehicle class to redefine this adjustment.
// default is to disallow backward-facing steering at low speed.
//

OpenSteer::Vec3 AITraffic_Vehicle::adjustRawSteeringForce (const Vec3& force,
                                                  const float /* deltaTime */)
{
    const float maxAdjustedSpeed = 0.2f * maxSpeed ();

    if ((speed () > maxAdjustedSpeed) || (force == Vec3::zero))
    {
        return force;
    }
    else
    {
        const float range = speed() / maxAdjustedSpeed;
        const float cosine = interpolate (pow (range, 20), 1.0f, -1.0f);
        return limitMaxDeviationAngle (force, cosine, forward());
    }
}


// ----------------------------------------------------------------------------
// xxx experimental 9-6-02
//
// apply a given braking force (for a given dt) to our momentum.
//
// (this is intended as a companion to applySteeringForce, but I'm not sure how
// well integrated it is.  It was motivated by the fact that "braking" (as in
// "capture the flag" endgame) by using "forward * speed * -rate" as a steering
// force was causing problems in adjustRawSteeringForce.  In fact it made it
// get NAN, but even if it had worked it would have defeated the braking.
//

void AITraffic_Vehicle::applyBrakingForce (const float rate, const float deltaTime)
{
    const float rawBraking = speed () * rate;
    const float clipBraking = ((rawBraking < maxForce ()) ?
                               rawBraking :
                               maxForce ());

    setSpeed (speed () - (clipBraking * deltaTime));
}


// ----------------------------------------------------------------------------
// apply a given steering force to our momentum,
// adjusting our orientation to maintain velocity-alignment.

void AITraffic_Vehicle::applySteeringForce (const Vec3& force, const float elapsedTime)
{
    const Vec3 adjustedForce = adjustRawSteeringForce (force, elapsedTime);

    // enforce limit on magnitude of steering force
    const Vec3 clippedForce = adjustedForce.truncateLength (maxForce ());

    // compute acceleration and velocity
    Vec3 newAcceleration = (clippedForce / mass());
    Vec3 newVelocity = velocity();

    // damp out abrupt changes and oscillations in steering acceleration
    // (rate is proportional to time step, then clipped into useful range)
    if (elapsedTime > 0)
    {
        const float smoothRate = clip (9 * elapsedTime, 0.15f, 0.4f);
        blendIntoAccumulator (smoothRate,
                              newAcceleration,
                              _smoothedAcceleration);
    }

    // Euler integrate (per frame) acceleration into velocity
    newVelocity += _smoothedAcceleration * elapsedTime;

    // enforce speed limit
    newVelocity = newVelocity.truncateLength (maxSpeed ());

    // update Speed
    setSpeed (newVelocity.length());

    // Euler integrate (per frame) velocity into position
	setPosition (getPosition() + (Ogre::Vector3(newVelocity.x, newVelocity.y, newVelocity.z)* elapsedTime));

    // regenerate local space (by default: align vehicle's forward axis with
    // new velocity, but this behavior may be overridden by derived classes.)
    regenerateLocalSpace (newVelocity, elapsedTime);

    // maintain path curvature information
    measurePathCurvature (elapsedTime);

    // running average of recent positions
    blendIntoAccumulator (elapsedTime * 0.06f, // QQQ
                          position (),
                          _smoothedPosition);
}


// ----------------------------------------------------------------------------
// the default version: keep FORWARD parallel to velocity, change UP as
// little as possible.
//
// parameter names commented out to prevent compiler warning from "-W"

void AITraffic_Vehicle::regenerateLocalSpace (const Vec3& newVelocity,
                                                const float /* elapsedTime */)
{
    // adjust orthonormal basis vectors to be aligned with new velocity
    if (speed() > 0) regenerateOrthonormalBasisUF (newVelocity / speed());
}


// ----------------------------------------------------------------------------
// alternate version: keep FORWARD parallel to velocity, adjust UP according
// to a no-basis-in-reality "banking" behavior, something like what birds and
// airplanes do

void AITraffic_Vehicle::regenerateLocalSpaceForBanking (const Vec3& newVelocity,
                                                          const float elapsedTime)
{
    // the length of this global-upward-pointing vector controls the vehicle's
    // tendency to right itself as it is rolled over from turning acceleration
    const Vec3 globalUp (0, 0.2f, 0);

    // acceleration points toward the center of local path curvature, the
    // length determines how much the vehicle will roll while turning
    const Vec3 accelUp = _smoothedAcceleration * 0.05f;

    // combined banking, sum of UP due to turning and global UP
    const Vec3 bankUp = accelUp + globalUp;

    // blend bankUp into vehicle's UP basis vector
    const float smoothRate = elapsedTime * 3;
    Vec3 tempUp = up();
    blendIntoAccumulator (smoothRate, bankUp, tempUp);
    setUp (tempUp.normalize());

    // adjust orthonormal basis vectors to be aligned with new velocity
    if (speed() > 0) regenerateOrthonormalBasisUF (newVelocity / speed());
}


// ----------------------------------------------------------------------------
// measure path curvature (1/turning-radius), maintain smoothed version

void AITraffic_Vehicle::measurePathCurvature (const float elapsedTime)
{
    if (elapsedTime > 0)
    {
        const Vec3 dP = _lastPosition - position ();
        const Vec3 dF = (_lastForward - forward ()) / dP.length ();
        const Vec3 lateral = dF.perpendicularComponent (forward ());
        const float sign = (lateral.dot (side ()) < 0) ? 1.0f : -1.0f;
        _curvature = lateral.length() * sign;
        blendIntoAccumulator (elapsedTime * 4.0f,
                              _curvature,
                              _smoothedCurvature);
        _lastForward = forward ();
        _lastPosition = position ();
    }
}

// ----------------------------------------------------------------------------
// predict position of this vehicle at some time in the future
// (assumes velocity remains constant, hence path is a straight line)
//

OpenSteer::Vec3 AITraffic_Vehicle::predictFuturePosition (const float predictionTime) const
{
    return position() + (velocity() * predictionTime);
}

Ogre::Vector3 AITraffic_Vehicle::getPosition()
{
	OpenSteer::AbstractVehicle* vehicle = this;
	return Ogre::Vector3(vehicle->position().x,vehicle->position().y,vehicle->position().z);
}

void AITraffic_Vehicle::setPosition(Ogre::Vector3 newPos)
{
//	mEntityNode->setPosition(newPos);
	static_cast<AbstractVehicle*>(this)->setPosition(OpenSteer::Vec3(newPos.x, newPos.y, newPos.z));
}

Ogre::Vector3 AITraffic_Vehicle::getDirection()
{
	OpenSteer::AbstractVehicle* vehicle = this;
	return Ogre::Vector3(vehicle->forward().x, vehicle->forward().y, vehicle->forward().z);
}

void AITraffic_Vehicle::update(const float currentTime, const float elapsedTime)
{
	OpenSteer::AbstractVehicle* vehicle = this;

	// Apply Steering forces
	// ---------------------

		int intTurningness = 6;

		const OpenSteer::Vec3 wander2d = steerForWander(elapsedTime * 24).setYtoZero();
		const OpenSteer::Vec3 steer = forward() + (wander2d * intTurningness);

		applySteeringForce (steer, elapsedTime);
/*
	mEntityNode->setPosition(Ogre::Vector3(vehicle->position().x,vehicle->position().y,vehicle->position().z));

	Ogre::Vector3 forwDir = Ogre::Vector3(vehicle->forward().x, vehicle->forward().y, vehicle->forward().z);

	mEntityNode->setDirection(forwDir, Ogre::SceneNode::TS_WORLD, mForwardVector);  

	if (hasAnimation)
		mEntityAnimState->addTime(elapsedTime * 1.0);
*/
}
