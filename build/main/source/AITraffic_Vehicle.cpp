#ifdef OPENSTEER

#include "AITraffic_Vehicle.h"

AITraffic_Vehicle::AITraffic_Vehicle()// : map (makeMap ()), path (makePath ())
{
	path_direction = 1;
//	makePath();

/* old code --
	reset();
	demoSelect = 2;
        // to compute mean time between collisions
        sumOfCollisionFreeTimes = 0;
        countOfCollisionFreeTimes = 0;

        // keep track for reliability statistics
        collisionLastTime = false;
//        timeOfLastCollision = clock.getTotalSimulationTime ();

        // keep track of average speed
        totalDistance = 0;
        totalTime = 0;

        // keep track of path following failure rate
        pathFollowTime = 0;
        pathFollowOffTime = 0;

        // innitialize counters for various performance data
        stuckCount = 0;
        stuckCycleCount = 0;
        stuckOffPathCount = 0;
        lapsStarted = 0;
        lapsFinished = 0;
        hintGivenCount = 0;
        hintTakenCount = 0;

        // follow the path "upstream or downstream" (+1/-1)
        pathFollowDirection = 1;

        // use curved prediction and incremental steering:
        curvedSteering = true;
        incrementalSteering = true;

        // 10 seconds with 200 points along the trail
//        setTrailParameters (10, 200);
*/
}

AITraffic_Vehicle::~AITraffic_Vehicle()
{
}

void AITraffic_Vehicle::reset (void)
{
	wp_idx = 0;
	advanceToNextWayPoint();
	speed = 0;

/* -- old code
	// reset LocalSpace state
	resetLocalSpace ();
	static_cast<AbstractVehicle*>(this)->resetLocalSpace();

        // reset the underlying vehicle class
        resetLocalSpace ();

            // reset SteerLibraryMixin state
            // (XXX this seems really fragile, needs to be redesigned XXX)
            SimpleVehicle_2IMI::reset ();

            setRadius (0.5f);     // size of bounding sphere

            setMaxForce (0.1f);   // steering force is clipped to this magnitude
            setMaxSpeed (1.0f);   // velocity is clipped to this magnitude

            // reset bookkeeping to do running averages of these quanities
            resetSmoothedPosition ();
            resetSmoothedCurvature ();
            resetSmoothedAcceleration ();

        // initially stopped
        setSpeed (0);

        // Assume top speed is 20 meters per second (44.7 miles per hour).
        // This value will eventually be supplied by a higher level module.
        setMaxSpeed (20);

        // steering force is clipped to this magnitude
        setMaxForce (maxSpeed () * 0.4f);

        // vehicle is 2 meters wide and 3 meters long
        halfWidth = 1.0f;
        halfLength = 1.5f;

        // init dynamically controlled radius
        adjustVehicleRadiusForSpeed ();

        // not previously avoiding
        annotateAvoid = Vec3::zero;

        // prevent long streaks due to teleportation 
//        clearTrailHistory ();

        // first pass at detecting "stuck" state
        stuck = false;

        // QQQ need to clean up this hack
        qqqLastNearestObstacle = Vec3::zero;

        // master look ahead (prediction) time
        baseLookAheadTime = 3;

        if (demoSelect == 2 && false)
        {
            lapsStarted++;
            const float s = worldSize;
            const float d = (float) pathFollowDirection;
            static_cast<AbstractVehicle*>(this)->setPosition (Vec3 (s * d * 0.6f, 0, s * -0.4f));
            regenerateOrthonormalBasisUF (Vec3::side * d);
        }

        // reset bookeeping to detect stuck cycles
        resetStuckCycleDetection ();

        // assume no previous steering
        currentSteering = Vec3::zero;

        // assume normal running state
        dtZero = false;

        // QQQ temporary global QQQoaJustScraping
        QQQoaJustScraping = false;

        // state saved for speedometer
//      annoteMaxRelSpeed = annoteMaxRelSpeedCurve = annoteMaxRelSpeedPath = 0;
//      annoteMaxRelSpeed = annoteMaxRelSpeedCurve = annoteMaxRelSpeedPath = 1;
*/
}

/*
void AITraffic_Vehicle::update (const float currentTime, const float elapsedTime)
{
		updateSimple(currentTime, elapsedTime);
		return;

        // take note when current dt is zero (as in paused) for stat counters
        dtZero = (elapsedTime == 0);

        // pretend we are bigger when going fast
        adjustVehicleRadiusForSpeed ();

        // state saved for speedometer
//      annoteMaxRelSpeed = annoteMaxRelSpeedCurve = annoteMaxRelSpeedPath = 0;
        annoteMaxRelSpeed = annoteMaxRelSpeedCurve = annoteMaxRelSpeedPath = 1;

        // determine combined steering
        Vec3 steering;
        const bool offPath = !bodyInsidePath ();
//        if (stuck || offPath || detectImminentCollision ())
		if (false)
        {
            // bring vehicle to a stop if we are stuck (newly or previously
            // stuck, because off path or collision seemed imminent)
            // (QQQ combine with stuckCycleCount code at end of this function?)
//          applyBrakingForce (curvedSteering ? 3 : 2, elapsedTime); // QQQ
            applyBrakingForce ((curvedSteering?3.0f:2.0f), elapsedTime); // QQQ
            // count "off path" events
            if (offPath && !stuck && (demoSelect == 2)) stuckOffPathCount++;
            stuck = true;

            // QQQ trying to prevent "creep" during emergency stops
            resetSmoothedAcceleration ();
            currentSteering = Vec3::zero;
        }
        else
        {
            // determine steering for obstacle avoidance (save for annotation)

            const Vec3 avoid = annotateAvoid = 
                steerToAvoidObstaclesOnMap (lookAheadTimeOA (),
                                            *map,
                                            hintForObstacleAvoidance ());
            const bool needToAvoid = avoid != Vec3::zero;

            // any obstacles to avoid?
//            if (needToAvoid)
            if (false)
            {
                // slow down and turn to avoid the obstacles
                const float targetSpeed =((curvedSteering && QQQoaJustScraping)
                                          ? maxSpeedForCurvature () : 0);
                annoteMaxRelSpeed = targetSpeed / maxSpeed ();
                const float avoidWeight = 3 + (3 * relativeSpeed ()); // ad hoc
//                steering = avoid * avoidWeight;
                steering += steerForTargetSpeed (targetSpeed);
            }
            else
            {
                // otherwise speed up and...
                steering = steerForTargetSpeed (maxSpeedForCurvature ());
				Ogre::LogManager::getSingleton().logMessage("TARGET SPEED: "+Ogre::StringConverter::toString(steering.x)+" "+Ogre::StringConverter::toString(steering.y)+" "+Ogre::StringConverter::toString(steering.z));

                // wander for demo 1
                if (demoSelect == 1)
                {
                    const Vec3 wander = steerForWander (elapsedTime);
                    const Vec3 flat = wander.setYtoZero ();
                    const Vec3 weighted = flat.truncateLength (maxForce()) * 6;
//imike                    const Vec3 a = position() + Vec3 (0, 0.2f, 0);
//                    annotationLine (a, a + (weighted * 0.3f), gWhite);
                    steering += weighted;
                }

                // follow the path in demo 2
                if (demoSelect == 2)
                {

                    const Vec3 pf = steerToFollowPath (pathFollowDirection,
                                                       lookAheadTimePF (),
                                                       *path);

//					Vec3 pf = Vec3::zero;
                    if (pf != Vec3::zero)
                    {
						Ogre::LogManager::getSingleton().logMessage("STEER PF: "+Ogre::StringConverter::toString(pf.x)+" "+Ogre::StringConverter::toString(pf.y)+" "+Ogre::StringConverter::toString(pf.z));
                        // steer to remain on path
                        if (pf.dot (forward()) < 0)
							{
								Ogre::LogManager::getSingleton().logMessage("--STEER TO REMAIN ON PATH");
								steering = pf;
								Ogre::LogManager::getSingleton().logMessage("STEER REMAIN: "+Ogre::StringConverter::toString(steering.x)+" "+Ogre::StringConverter::toString(steering.y)+" "+Ogre::StringConverter::toString(steering.z));
							}
                        else
							{
								steering = pf + steering;
								Ogre::LogManager::getSingleton().logMessage("--STEER TO CORRECT: "+Ogre::StringConverter::toString(steering.x)+" "+Ogre::StringConverter::toString(steering.y)+" "+Ogre::StringConverter::toString(steering.z));
							}
                    }
                    else
                    {
                        // path aligment: when neither obstacle avoidance nor
                        // path following is required, align with path segment
						Ogre::LogManager::getSingleton().logMessage("--PATH ALIGNMENT");
                        const Vec3 pathHeading =
                            path->tangentAt (position (),
                                             pathFollowDirection);
                        {
                            const Vec3 b = (position () +
                                            (up () * 0.2f) +
                                            (forward () * halfLength * 1.4f));
                            const float l = 2;
//                            annotationLine (b, b + (forward ()  * l), gCyan);
  //                          annotationLine (b, b + (pathHeading * l), gCyan);
                        }
                        steering += (steerTowardHeading(pathHeading) *
                                     (path->nearWaypoint (position ()) ?
                                      0.5f : 0.1f));

                    }
                }
            }
        }

        if (!stuck) // <<-FAIL
        {
            // convert from absolute to incremental steering signal
//            if (incrementalSteering)
//                steering = convertAbsoluteToIncrementalSteering (steering, elapsedTime);
            // enforce minimum turning radius
            steering = adjustSteeringForMinimumTurningRadius (steering);
        }

        // apply selected steering force to vehicle, record data
        applySteeringForce (steering, elapsedTime);
//        collectReliabilityStatistics (currentTime, elapsedTime);

        // detect getting stuck in cycles -- we are moving but not
        // making progress down the route (annotate smoothedPosition)
        if (demoSelect == 2 && false)
        {
            const bool circles = weAreGoingInCircles ();
            if (circles && !stuck) stuckCycleCount++;
            if (circles) stuck = true;
//            annotationCircleOrDisk (0.5, up(), smoothedPosition (),
  //                                  gWhite, 12, circles, false);
        }

        // annotation
//        perFrameAnnotation ();
//        recordTrailVertex (currentTime, position());

}


void AITraffic_Vehicle::adjustVehicleRadiusForSpeed (void)
{
	 const float minRadius = sqrtXXX(square(halfWidth)+square(halfLength));
        const float safetyMargin = (curvedSteering ?
                                    interpolate (relativeSpeed(), 0.0f, 1.5f) :
                                    0.0f);
        setRadius (minRadius + safetyMargin);
}


void AITraffic_Vehicle::collectReliabilityStatistics (const float currentTime, const float elapsedTime)
{
	// detect obstacle avoidance failure and keep statistics
	collisionDetected = map->scanLocalXZRectangle (*this,
                                                       -halfWidth, halfWidth,
                                                       -halfLength,halfLength);

	// record stats to compute mean time between collisions
	const float timeSinceLastCollision = currentTime - timeOfLastCollision;
	if (collisionDetected && !collisionLastTime && (timeSinceLastCollision > 1))
        {
            std::ostringstream message;
            message << "collision after "<<timeSinceLastCollision<<" seconds";
//            OpenSteerDemo::printMessage (message);
            sumOfCollisionFreeTimes += timeSinceLastCollision;
            countOfCollisionFreeTimes++;
            timeOfLastCollision = currentTime;
        }
	collisionLastTime = collisionDetected;

	// keep track of average speed
	totalDistance += speed () * elapsedTime;
	totalTime += elapsedTime;

	// keep track of path following failure rate
	// QQQ for now, duplicating this code from the draw method:
	// if we are following a path but are off the path,
	// draw a red line to where we should be
	pathFollowTime += elapsedTime;
	if (! bodyInsidePath ()) pathFollowOffTime += elapsedTime;
    }

OpenSteer::Vec3 AITraffic_Vehicle::hintForObstacleAvoidance (void)
{
/*
	// are we heading roughly parallel to the current path segment?
	const OpenSteer::Vec3 p = position ();
	const OpenSteer::Vec3 pathHeading = path->tangentAt (p, pathFollowDirection);
	if (pathHeading.dot (forward ()) < 0.8f)
        {
            // if not, the "hint" is to turn to align with path heading
            const OpenSteer::Vec3 s = side () * halfWidth;
            const float f = halfLength * 2;
            return pathHeading;
        }
	else
        {
            // when there is a valid nearest obstacle position
            const OpenSteer::Vec3 obstacle = qqqLastNearestObstacle;
            const OpenSteer::Vec3 o = obstacle + (up () * 0.1f);
            if (obstacle != OpenSteer::Vec3::zero)
            {
                // get offset, distance from obstacle to its image on path
                float outside;
                const OpenSteer::Vec3 onPath = path->mapPointToPath (obstacle, outside);
                const OpenSteer::Vec3 offset = onPath - obstacle;
                const float offsetDistance = offset.length();

                // when the obstacle is inside the path tube
                if (outside < 0)
                {
                    // when near the outer edge of a sufficiently wide tube
                    const int segmentIndex =
                        path->indexOfNearestSegment (onPath);
                    const float segmentRadius = path->radii [segmentIndex];
                    const float w = halfWidth * 6;
                    const bool nearEdge = offsetDistance > w;
                    const bool wideEnough = segmentRadius > (w * 2);
                    if (nearEdge && wideEnough)
                    {
                        const float obstacleDistance = (obstacle - p).length();
                        const float range = speed () * lookAheadTimeOA ();
                        const float farThreshold = range * 0.8f;
                        const bool usableHint = obstacleDistance>farThreshold;
                        if (usableHint)
                        {
                            const OpenSteer::Vec3 q = p + (offset.normalize() * 5);
                            return offset;
                        }
                    }
                }
            }
        }
	// otherwise, no hint

	return OpenSteer::Vec3::zero;
}

// like steerToAvoidObstacles, but based on a BinaryTerrainMap indicating
// the possitions of impassible regions
//
OpenSteer::Vec3 AITraffic_Vehicle::steerToAvoidObstaclesOnMap (const float minTimeToCollision, const AITraffic_TerrainMap& map)
{
	return steerToAvoidObstaclesOnMap (minTimeToCollision, map, OpenSteer::Vec3::zero); // no steer hint
}


// given a map of obstacles (currently a global, binary map) steer so as
// to avoid collisions within the next minTimeToCollision seconds.
//
OpenSteer::Vec3 AITraffic_Vehicle::steerToAvoidObstaclesOnMap (const float minTimeToCollision,
															   const AITraffic_TerrainMap& map,
																const OpenSteer::Vec3& steerHint)
{
	return OpenSteer::Vec3::zero;

	const float spacing = map.minSpacing() / 2;
	const float maxSide = radius();
	const float maxForward = minTimeToCollision * speed();
	const int maxSamples = (int) (maxForward / spacing);
	const OpenSteer::Vec3 step = forward () * spacing;
	const OpenSteer::Vec3 fOffset = position ();
	OpenSteer::Vec3 sOffset;
	float s = spacing / 2;

	const int infinity = 9999; // qqq
	int nearestL = infinity;
	int nearestR = infinity;
	int nearestWL = infinity;
	int nearestWR = infinity;
	OpenSteer::Vec3 nearestO;
	wingDrawFlagL = false;
	wingDrawFlagR = false;

	const bool hintGiven = steerHint != OpenSteer::Vec3::zero;
	if (hintGiven && !dtZero) hintGivenCount++;
	// QQQ temporary global QQQoaJustScraping
	QQQoaJustScraping = true;

	const float signedRadius = 1 / nonZeroCurvatureQQQ ();
	const OpenSteer::Vec3 localCenterOfCurvature = side () * signedRadius;
	const OpenSteer::Vec3 center = position () + localCenterOfCurvature;
	const float sign = signedRadius < 0 ? 1.0f : -1.0f;
	const float arcRadius = signedRadius * -sign;
	const float twoPi = 2 * OPENSTEER_M_PI;
	const float circumference = twoPi * arcRadius;
	const float rawLength = speed() * minTimeToCollision * sign;
	const float fracLimit = 1.0f / 6.0f;
	const float distLimit = circumference * fracLimit;
	const float arcLength = arcLengthLimit (rawLength, distLimit);
	const float arcAngle = twoPi * arcLength / circumference;

	// XXX temp annotation to show limit on arc angle
	if (curvedSteering)
		{
            if ((speed() * minTimeToCollision) > (circumference * fracLimit))
            {
                const float q = twoPi * fracLimit;
                const OpenSteer::Vec3 fooz = position () - center;
                const OpenSteer::Vec3 booz = fooz.rotateAboutGlobalY (sign * q);
            }
        }

        // assert loops will terminate
	assert (spacing > 0);

        // scan corridor straight ahead of vehicle,
        // keep track of nearest obstacle on left and right sides
	while (s < maxSide)
        {
            sOffset = side() * s;
            s += spacing;
            const OpenSteer::Vec3 lOffset = fOffset + sOffset;
            const OpenSteer::Vec3 rOffset = fOffset - sOffset;

            OpenSteer::Vec3 lObsPos, rObsPos;

            const int L = (curvedSteering ? 
                           (int) (scanObstacleMap (lOffset,
                                                   center,
                                                   arcAngle,
                                                   maxSamples,
												   0,
                                                   OpenSteer::Vec3(0,0,0),
                                                   OpenSteer::Vec3(0,0,0),
                                                   lObsPos)
                                  / spacing) :
                           map.scanXZray (lOffset, step, maxSamples));
            const int R = (curvedSteering ? 
                           (int) (scanObstacleMap (rOffset,
                                                    center,
                                                   arcAngle,
                                                   maxSamples,
                                                   0,
                                                   OpenSteer::Vec3(0,0,0),
                                                   OpenSteer::Vec3(0,0,0),
                                                   rObsPos)
                                  / spacing) :
                           map.scanXZray (rOffset, step, maxSamples));

            if ((L > 0) && (L < nearestL))
            {
                nearestL = L;
                if (L < nearestR) nearestO = ((curvedSteering) ?
                                              lObsPos :
                                              lOffset + ((float)L * step));
            }
            if ((R > 0) && (R < nearestR))
            {
                nearestR = R;
                if (R < nearestL) nearestO = ((curvedSteering) ?
                                              rObsPos :
                                              rOffset + ((float)R * step));
            }

            if (!curvedSteering)
            {
                annotateAvoidObstaclesOnMap (lOffset, L, step);
                annotateAvoidObstaclesOnMap (rOffset, R, step);
            }

            if (curvedSteering)
            {
                // QQQ temporary global QQQoaJustScraping
                const bool outermost = s >= maxSide;
                const bool eitherSide = (L > 0) || (R > 0);
                if (!outermost && eitherSide) QQQoaJustScraping = false;
            }
        }
	qqqLastNearestObstacle = nearestO;

        // scan "wings"
        {
            const int wingScans = 4;
            // see duplicated code at: QQQ draw sensing "wings"
            // QQQ should be a parameter of this method
            const OpenSteer::Vec3 wingWidth = side() * wingSlope () * maxForward;

            const OpenSteer::Vec3 beforeColor (0.75f, 0.9f, 0.0f);  // for annotation
            const OpenSteer::Vec3 afterColor  (0.9f,  0.5f, 0.0f);  // for annotation

            for (int i=1; i<=wingScans; i++)
            {
                const float fraction = (float)i / (float)wingScans;
                const OpenSteer::Vec3 endside = sOffset + (wingWidth * fraction);
                const OpenSteer::Vec3 corridorFront = forward() * maxForward;

                // "loop" from -1 to 1
                for (int j = -1; j < 2; j+=2)
                {
                    float k = (float)j; // prevent VC7.1 warning
                    const OpenSteer::Vec3 start = fOffset + (sOffset * k);
                    const OpenSteer::Vec3 end = fOffset + corridorFront + (endside * k);
                    const OpenSteer::Vec3 ray = end - start;
                    const float rayLength = ray.length();
                    const OpenSteer::Vec3 step = ray * spacing / rayLength;
                    const int raySamples = (int) (rayLength / spacing);
                    const float endRadius =
                        wingSlope () * maxForward * fraction *
                        (signedRadius < 0 ? 1 : -1) * (j==1?1:-1);
                    OpenSteer::Vec3 ignore;
                    const int scan = (curvedSteering ?
                                      (int) (scanObstacleMap (start,
                                                              center,
                                                              arcAngle,
                                                              raySamples,
                                                              endRadius,
                                                              OpenSteer::Vec3(0,0,0),
                                                              OpenSteer::Vec3(0,0,0),
                                                              ignore)
                                             / spacing) :
                                      map.scanXZray (start, step, raySamples));

                    if (!curvedSteering)
                        annotateAvoidObstaclesOnMap (start,scan,step);

                    if (j==1) 
                    {
                        if ((scan > 0) && (scan < nearestWL)) nearestWL = scan;
                    }
                    else
                    {
                        if ((scan > 0) && (scan < nearestWR)) nearestWR = scan;
                    }
                }
            }
            wingDrawFlagL = nearestWL != infinity;
            wingDrawFlagR = nearestWR != infinity;
        }

        // for annotation
        savedNearestWR = (float) nearestWR;
        savedNearestR  = (float) nearestR;
        savedNearestL  = (float) nearestL;
        savedNearestWL = (float) nearestWL;

        // flags for compound conditions, used below
        const bool obstacleFreeC  = nearestL==infinity && nearestR==infinity;
        const bool obstacleFreeL  = nearestL==infinity && nearestWL==infinity;
        const bool obstacleFreeR  = nearestR==infinity && nearestWR==infinity;
        const bool obstacleFreeWL = nearestWL==infinity;
        const bool obstacleFreeWR = nearestWR==infinity;
        const bool obstacleFreeW  = obstacleFreeWL && obstacleFreeWR;

        // when doing curved steering and we have already detected "just
        // scarping" but neither wing is free, recind the "just scarping"
        // QQQ temporary global QQQoaJustScraping
        const bool JS = curvedSteering && QQQoaJustScraping;
        const bool cancelJS = !obstacleFreeWL && !obstacleFreeWR;
        if (JS && cancelJS) QQQoaJustScraping = false;


        // ----------------------------------------------------------
        // now we have measured everything, decide which way to steer
        // ----------------------------------------------------------


        // no obstacles found on path, return zero steering
        if (obstacleFreeC)
        {
            qqqLastNearestObstacle = OpenSteer::Vec3::zero;

            // qqq  this may be in the wrong place (what would be the right
            // qqq  place?!) but I'm trying to say "even if the path is
            // qqq  clear, don't go too fast when driving between obstacles
            if (obstacleFreeWL || obstacleFreeWR || relativeSpeed () < 0.7f)
                return OpenSteer::Vec3::zero;
            else
                return -forward ();
        }

        // if the nearest obstacle is way out there, take hint if any
//      if (hintGiven && (minXXX (nearestL, nearestR) > (maxSamples * 0.8f)))
        if (hintGiven && (minXXX ((float)nearestL, (float)nearestR) >
                          (maxSamples * 0.8f)))
        {
            annotationHintWasTaken ();
            if (steerHint.dot(side())>0) return side();else return -side();
        }

        // QQQ experiment 3-9-04
        //
        // since there are obstacles ahead, if we are already near
        // maximum curvature, we MUST turn in opposite direction
        //
        // are we turning more sharply than the minimum turning radius?
        // (code from adjustSteeringForMinimumTurningRadius)
        const float maxCurvature = 1 / (minimumTurningRadius () * 1.2f);
        if (absXXX (curvature ()) > maxCurvature)
        {
            return side () * sign;
        }

        if (obstacleFreeL) return side();
        if (obstacleFreeR) return -side();

        // if wings are clear, turn away from nearest obstacle straight ahead
        if (obstacleFreeW)
        {
            // distance to obs on L and R side of corridor roughtly the same
            const bool same = absXXX (nearestL - nearestR) < 5; // within 5
            // if they are about the same and a hint is given, use hint
            if (same && hintGiven)
            {
                annotationHintWasTaken ();
                if (steerHint.dot(side())>0) return side();else return -side();
            }
            else
            {
                // otherwise steer toward the less cluttered side
                if (nearestL > nearestR) return side(); else return -side();
            }
        }

        // if the two wings are about equally clear and a steering hint is
        // provided, use it
        const bool equallyClear = absXXX (nearestWL-nearestWR) < 2; // within 2
        if (equallyClear && hintGiven)
        {
            annotationHintWasTaken ();
            if (steerHint.dot(side()) > 0) return side(); else return -side();
        }

        // turn towards the side whose "wing" region is less cluttered
        // (the wing whose nearest obstacle is furthest away)
        if (nearestWL > nearestWR) return side(); else return -side();
    }



// QQQ reconsider calling sequence
// called when steerToAvoidObstaclesOnMap decides steering is required
// (default action is to do nothing, layered classes can overload it)
// virtual void annotateAvoidObstaclesOnMap (const OpenSteer::Vec3& scanOrigin,
//                                           int scanIndex,
//                                           const OpenSteer::Vec3& scanStep)
// {
// }
void AITraffic_Vehicle::annotateAvoidObstaclesOnMap (const OpenSteer::Vec3& scanOrigin,
                                      int scanIndex,
                                      const OpenSteer::Vec3& scanStep)
{
	if (scanIndex > 0)
		{
			const OpenSteer::Vec3 hit = scanOrigin + (scanStep * (float) scanIndex);
	}
}


void AITraffic_Vehicle::annotationHintWasTaken (void)
{
	if (!dtZero) hintTakenCount++;

	const float r = halfWidth * 0.9f;
	const OpenSteer::Vec3 ff = forward () * r;
	const OpenSteer::Vec3 ss = side () * r;
	const OpenSteer::Vec3 pp = position () + (up () * 0.2f);
}


// scan across the obstacle map along a given arc
// (possibly with radius adjustment ramp)
// returns approximate distance to first obstacle found
//
// QQQ 1: this calling sequence does not allow for zero curvature case
// QQQ 2: in library version of this, "map" should be a parameter
// QQQ 3: instead of passing in colors, call virtual annotation function?
// QQQ 4: need flag saying to continue after a hit, for annotation
// QQQ 5: I needed to return both distance-to and position-of the first
//        obstacle. I added returnObstaclePosition but maybe this should
//        return a "scan results object" with a flag for obstacle found,
//        plus distant and position if so.
//
float AITraffic_Vehicle::scanObstacleMap (const OpenSteer::Vec3& start,
                           const OpenSteer::Vec3& center,
                           const float arcAngle,
                           const int segments,
                           const float endRadiusChange,
                           const OpenSteer::Vec3& beforeColor,
                           const OpenSteer::Vec3& afterColor,
                           OpenSteer::Vec3& returnObstaclePosition)
{
	// "spoke" is initially the vector from center to start,
	// which is then rotated step by step around center
	OpenSteer::Vec3 spoke = start - center;
	// determine the angular step per segment
	const float step = arcAngle / segments;
	// store distance to, and position of first obstacle
	float obstacleDistance = 0;
	returnObstaclePosition = OpenSteer::Vec3::zero;
	// for spiral "ramps" of changing radius
	const float startRadius = (endRadiusChange == 0) ? 0 : spoke.length(); 

	// traverse each segment along arc
	float sin=0, cos=0;
	OpenSteer::Vec3 oldPoint = start;
	bool obstacleFound = false;
	for (int i = 0; i < segments; i++)
        {
            // rotate "spoke" to next step around circle
            // (sin and cos values get filled in on first call)
            spoke = spoke.rotateAboutGlobalY (step, sin, cos);

            // for spiral "ramps" of changing radius
            const float adjust = ((endRadiusChange == 0) ?
                                  1.0f :
                                  interpolate ((float)(i+1) / (float)segments,
                                               1.0f,
                                               (maxXXX (0,
                                                        (startRadius +
                                                         endRadiusChange))
                                                / startRadius)));

            // construct new scan point: center point, offset by rotated
            // spoke (possibly adjusting the radius if endRadiusChange!=0)
            const OpenSteer::Vec3 newPoint = center + (spoke * adjust);

            // once an obstacle if found "our work here is done" -- continue
            // to loop only for the sake of annotation (make that optional?)
            if (obstacleFound)
            {
            }
            else
            {
                // no obstacle found on this scan so far,
                // scan map along current segment (a chord of the arc)
                const OpenSteer::Vec3 offset = newPoint - oldPoint;
                const float d2 = offset.length() * 2;

                // when obstacle found: set flag, save distance and position
                if (! map->isPassable (newPoint))
                {
                    obstacleFound = true;
                    obstacleDistance = d2 * 0.5f * (i+1);
                    returnObstaclePosition = newPoint;
                }
            }
            // save new point for next time around loop
            oldPoint = newPoint;
        }
        // return distance to first obstacle (or zero if none found)
	return obstacleDistance;
}


bool AITraffic_Vehicle::detectImminentCollision (void)
{
	bool returnFlag = false;

	// QQQ  this should be integrated into steerToAvoidObstaclesOnMap
	// QQQ  since it shares so much infrastructure
	// QQQ  less so after changes on 3-16-04
	const float spacing = map->minSpacing() / 2;
	const float maxSide = halfWidth + spacing;
	const float minDistance = curvedSteering ? 2.0f : 2.5f; // meters
	const float predictTime = curvedSteering ? .75f : 1.3f; // seconds
	const float maxForward =
            speed () * combinedLookAheadTime (predictTime, minDistance);
	const OpenSteer::Vec3 step = forward () * spacing;
	float s = curvedSteering ? (spacing / 4) : (spacing / 2);

	const float signedRadius = 1 / nonZeroCurvatureQQQ ();
	const OpenSteer::Vec3 localCenterOfCurvature = side () * signedRadius;
	const OpenSteer::Vec3 center = position () + localCenterOfCurvature;
	const float sign = signedRadius < 0 ? 1.0f : -1.0f;
	const float arcRadius = signedRadius * -sign;
	const float twoPi = 2 * OPENSTEER_M_PI;
	const float circumference = twoPi * arcRadius;
	const OpenSteer::Vec3 qqqLift (0, 0.2f, 0);
	OpenSteer::Vec3 ignore;

        // scan region ahead of vehicle
	while (s < maxSide)
        {
            const OpenSteer::Vec3 sOffset = side() * s;
            const OpenSteer::Vec3 lOffset = position () + sOffset;
            const OpenSteer::Vec3 rOffset = position () - sOffset;
            const float bevel = 0.3f;
            const float fraction = s / maxSide;
            const float scanDist = (halfLength +
                                    interpolate (fraction,
                                                 maxForward,
                                                 maxForward * bevel));
            const float angle = (scanDist * twoPi * sign) / circumference;
            const int samples = (int) (scanDist / spacing);
            const int L = (curvedSteering ?
                           (int) (scanObstacleMap (lOffset + qqqLift,
                                                   center,
                                                   angle,
                                                   samples,
                                                   0,
                                                   OpenSteer::Vec3(0,0,0),
                                                   OpenSteer::Vec3(0,0,0),
                                                   ignore)
                                  / spacing) :
                           map->scanXZray (lOffset, step, samples));
            const int R = (curvedSteering ?
                           (int) (scanObstacleMap (rOffset + qqqLift,
                                                   center,
                                                   angle,
                                                   samples,
                                                   0,
                                                   OpenSteer::Vec3(0,0,0),
                                                   OpenSteer::Vec3(0,0,0),
                                                   ignore)
                                  / spacing) :
                           map->scanXZray (rOffset, step, samples));

            returnFlag = returnFlag || (L > 0);
            returnFlag = returnFlag || (R > 0);

            // annotation
            if (! curvedSteering)
            {
                const OpenSteer::Vec3 d (step * (float) samples);
            }

            // increment sideways displacement of scan line
            s += spacing;
        }

	return returnFlag;
}


// see comments at SimpleVehicle::predictFuturePosition, in this instance
// I just need the future position (not a LocalSpace), so I'll keep the
// calling sequence and just conditionalize its body
//
// this should be const, but easier for now to ignore that

OpenSteer::Vec3 AITraffic_Vehicle::predictFuturePosition (const float predictionTime)
{
	return OpenSteer::Vec3::zero;

        if (curvedSteering)
        {
            // QQQ this chunk of code is repeated in far too many places,
            // QQQ it has to be moved inside some utility
            // QQQ 
            // QQQ and now, worse, I rearranged it to try the "limit arc
            // QQQ angle" trick
            const float signedRadius = 1 / nonZeroCurvatureQQQ ();
            const OpenSteer::Vec3 localCenterOfCurvature = side () * signedRadius;
            const OpenSteer::Vec3 center = position () + localCenterOfCurvature;
            const float sign = signedRadius < 0 ? 1.0f : -1.0f;
            const float arcRadius = signedRadius * -sign;
            const float twoPi = 2 * OPENSTEER_M_PI;
            const float circumference = twoPi * arcRadius;
            const float rawLength = speed() * predictionTime * sign;
            const float arcLength = arcLengthLimit (rawLength,
                                                    circumference * 0.25f);
            const float arcAngle = twoPi * arcLength / circumference;

            const OpenSteer::Vec3 spoke = position () - center;
            const OpenSteer::Vec3 newSpoke = spoke.rotateAboutGlobalY (arcAngle);
            const OpenSteer::Vec3 prediction = newSpoke + center;

            return prediction;
        }
        else
        {
            return position() + (velocity() * predictionTime);
        }

    }


// QQQ experimental fix for arcLength limit in predictFuturePosition
// QQQ and steerToAvoidObstaclesOnMap.
//
// args are the intended arc length (signed!), and the limit which is
// a given (positive!) fraction of the arc's (circle's) circumference
//

float AITraffic_Vehicle::arcLengthLimit (const float length, const float limit)
{
	if (length > 0) return minXXX (length, limit);
	else return -minXXX (-length, limit);
}

// this is a version of the one in SteerLibrary.h modified for "slow when
// heading off path".  I put it here because the changes were not
// compatible with Pedestrians.cpp.  It needs to be merged back after
// things settle down.
//
// its been modified in other ways too (such as "reduce the offset if
// facing in the wrong direction" and "increase the target offset to
// compensate the fold back") plus I changed the type of "path" from
// Pathway to AITraffic_Route to use methods like indexOfNearestSegment and
// dotSegmentUnitTangents
//
// and now its been modified again for curvature-based prediction
//
OpenSteer::Vec3 AITraffic_Vehicle::steerToFollowPath (const int direction,
                            const float predictionTime,
                            AITraffic_Route& path)
{
	return steerToFollowPathLinear (direction, predictionTime, path);
	return steerToFollowPathCurve (direction, predictionTime, path);
}

OpenSteer::Vec3 AITraffic_Vehicle::steerToFollowPathLinear (const int direction,
                                  const float predictionTime,
                                  AITraffic_Route& path)
{
	return OpenSteer::Vec3::zero;

	// our goal will be offset from our path distance by this amount
	const float pathDistanceOffset = direction * predictionTime * speed();

	// predict our future position
	const OpenSteer::Vec3 futurePosition = predictFuturePosition (predictionTime);

	// measure distance along path of our current and predicted positions
	const float nowPathDistance =
            path.mapPointToPathDistance (position ());

	// are we facing in the correction direction?
	const OpenSteer::Vec3 pathHeading = path.tangentAt(position()) * (float)direction;
	const bool correctDirection = pathHeading.dot (forward ()) > 0;

	// find the point on the path nearest the predicted future position
	// XXX need to improve calling sequence, maybe change to return a
	// XXX special path-defined object which includes two OpenSteer::Vec3s and a 
	// XXX bool (onPath,tangent (ignored), withinPath)
	float futureOutside;
	const OpenSteer::Vec3 onPath = path.mapPointToPath (futurePosition,futureOutside);

	// determine if we are currently inside the path tube
	float nowOutside;
	const OpenSteer::Vec3 nowOnPath = path.mapPointToPath (position (), nowOutside);

	// no steering is required if our present and future positions are
	// inside the path tube and we are facing in the correct direction
	const float m = -radius ();
	const bool whollyInside = (futureOutside < m) && (nowOutside < m);
	if (whollyInside && correctDirection)
        {
            // all is well, return zero steering
            return OpenSteer::Vec3::zero;
        }
	else
        {
            // otherwise we need to steer towards a target point obtained
            // by adding pathDistanceOffset to our current path position
            // (reduce the offset if facing in the wrong direction)
            const float targetPathDistance = (nowPathDistance + 
                                              (pathDistanceOffset *
                                               (correctDirection ? 1 : 0.1f)));
            OpenSteer::Vec3 target = path.mapPathDistanceToPoint (targetPathDistance);


            // if we are on one segment and target is on the next segment and
            // the dot of the tangents of the two segments is negative --
            // increase the target offset to compensate the fold back
            const int ip = path.indexOfNearestSegment (position ());
            const int it = path.indexOfNearestSegment (target);
            if (((ip + direction) == it) &&
                (path.dotSegmentUnitTangents (it, ip) < -0.1f))
            {
                const float newTargetPathDistance =
                    nowPathDistance + (pathDistanceOffset * 2);
                target = path.mapPathDistanceToPoint (newTargetPathDistance);
            }

            annotatePathFollowing (futurePosition,onPath,target,futureOutside);

            // if we are currently outside head directly in
            // (QQQ new, experimental, makes it turn in more sharply)
            if (nowOutside > 0) return steerForSeek (nowOnPath);

            // steering to seek target on path
            const OpenSteer::Vec3 seek = steerForSeek (target).truncateLength(maxForce());

            // return that seek steering -- except when we are heading off
            // the path (currently on path and future position is off path)
            // in which case we put on the brakes.
            if ((nowOutside < 0) && (futureOutside > 0))
                return (seek.perpendicularComponent (forward ()) -
                        (forward () * maxForce ()));
            else
                return seek;
		}

}


// Path following case for curved prediction and incremental steering
// (called from steerToFollowPath for the curvedSteering case)
//
// QQQ this does not handle the case when we AND futurePosition
// QQQ are outside, say when approach the path from far away
//
OpenSteer::Vec3 AITraffic_Vehicle::steerToFollowPathCurve (const int direction,
                                 const float predictionTime,
                                 AITraffic_Route& path)
{
	return OpenSteer::Vec3::zero;
	
	// predict our future position (based on current curvature and speed)
	const OpenSteer::Vec3 futurePosition = predictFuturePosition (predictionTime);
	// find the point on the path nearest the predicted future position
	float futureOutside;
	const OpenSteer::Vec3 onPath = path.mapPointToPath (futurePosition,futureOutside);
	const OpenSteer::Vec3 pathHeading = path.tangentAt (onPath, direction);
	const OpenSteer::Vec3 rawBraking = forward () * maxForce () * -1;
	const OpenSteer::Vec3 braking = ((futureOutside < 0) ? OpenSteer::Vec3::zero : rawBraking);
	//qqq experimental wrong-way-fixer
	float nowOutside;
	OpenSteer::Vec3 nowTangent;
	const OpenSteer::Vec3 p = position ();
	const OpenSteer::Vec3 nowOnPath = path.mapPointToPath (p, nowTangent, nowOutside);
	nowTangent *= (float)direction;
	const float alignedness = nowTangent.dot (forward ());

	// facing the wrong way?
	if (alignedness < 0)
        {

            // if nearly anti-parallel
            if (alignedness < -0.707f)
            {
                const OpenSteer::Vec3 towardCenter = nowOnPath - p;
                const OpenSteer::Vec3 turn = (towardCenter.dot (side ()) > 0 ?
                                   side () * maxForce () :
                                   side () * maxForce () * -1);
                return (turn + rawBraking);
            }
            else
            {
                return (steerTowardHeading(pathHeading).
                        perpendicularComponent(forward()) + braking);
            }
        }

        // is the predicted future position(+radius+margin) inside the path?
        if (futureOutside < -(radius () + 1.0f)) //QQQ
        {
            // then no steering is required
            return OpenSteer::Vec3::zero;
        }
        else
        {
            // otherwise determine corrective steering (including braking)

            // two cases, if entering a turn (a waypoint between path segments)
            if (path.nearWaypoint (onPath) && (futureOutside > 0))
            {
                // steer to align with next path segment
                return steerTowardHeading (pathHeading) + braking;
            }
            else
            {
                // otherwise steer away from the side of the path we
                // are heading for
                const OpenSteer::Vec3 pathSide = localRotateForwardToSide (pathHeading);
                const OpenSteer::Vec3 towardFP = futurePosition - onPath;
                const float whichSide = (pathSide.dot(towardFP)<0)?1.0f :-1.0f;
                return (side () * maxForce () * whichSide) + braking;
            }
  
 }

}


void AITraffic_Vehicle::perFrameAnnotation (void)
{
}

AITraffic_TerrainMap* AITraffic_Vehicle::makeMap (void)
{
	AITraffic_TerrainMap *newmap = new AITraffic_TerrainMap (Vec3::zero, worldSize, worldSize, (int)worldSize + 1);
//	newmap->clear();
	return map;
}


bool AITraffic_Vehicle::handleExitFromMap (void)
{

	// for path following, do wrap-around (teleport) and make new map
	const float px = position ().x;
	const float fx = forward ().x;
	const float ws = worldSize * 0.51f; // slightly past edge
	if (((fx > 0) && (px > ws)) || ((fx < 0) && (px < -ws)))
		{
			// bump counters
			lapsStarted++;
			lapsFinished++;

//			const OpenSteer::Vec3 camOffsetBefore = OpenSteerDemo::camera.position() - position ();

			// set position on other side of the map (set new X coordinate)
			AbstractVehicle::setPosition ((((px < 0) ? 1 : -1) *
                              ((worldSize * 0.5f) +
                               (speed() * lookAheadTimePF ()))),
                             AbstractVehicle::position ().y,
                             AbstractVehicle::position ().z);

			// reset bookeeping to detect stuck cycles
			resetStuckCycleDetection ();

			// new camera position and aimpoint to compensate for teleport
//			OpenSteerDemo::camera.target = position ();
//			OpenSteerDemo::camera.setPosition (position () + camOffsetBefore);

			// make camera jump immediately to new position
//			OpenSteerDemo::camera.doNotSmoothNextMove ();

            return true; 
		}

	return false;
}


float AITraffic_Vehicle::relativeSpeed (void) const 
{
	return speed () / maxSpeed ();
}


float AITraffic_Vehicle::wingSlope (void)
{
	return interpolate (relativeSpeed (),(curvedSteering ? 0.3f : 0.35f), 0.06f);
}


void AITraffic_Vehicle::resetStuckCycleDetection (void)
{
//	resetSmoothedPosition (position () + (forward () * -80)); // qqq
}


// QQQ just a stop gap, not quite right
// (say for example we were going around a circle with radius > 10)
bool AITraffic_Vehicle::weAreGoingInCircles (void)
{
	const OpenSteer::Vec3 offset = smoothedPosition () - position ();
	return offset.length () < 10;
}


float AITraffic_Vehicle::lookAheadTimeOA (void) const
{
	const float minTime = (baseLookAheadTime *
                               (curvedSteering ?
                                interpolate (relativeSpeed(), 0.4f, 0.7f) :
                                0.66f));
	return combinedLookAheadTime (minTime, 3);
}

float AITraffic_Vehicle::lookAheadTimePF (void) const
{
	return combinedLookAheadTime (baseLookAheadTime, 3);
}

// QQQ maybe move to SimpleVehicle ?
// compute a "look ahead time" with two components, one based on
// minimum time to (say) a collision and one based on minimum distance
// arg 1 is "seconds into the future", arg 2 is "meters ahead"
float AITraffic_Vehicle::combinedLookAheadTime (float minTime, float minDistance) const
{
	if (speed () == 0) return 0;
	return maxXXX (minTime, minDistance / speed ());
}


    // is vehicle body inside the path?
    // (actually tests if all four corners of the bounbding box are inside)
    //
bool AITraffic_Vehicle::bodyInsidePath (void)
{
	const OpenSteer::Vec3 bbSide = side () * halfWidth;
	const OpenSteer::Vec3 bbFront = forward () * halfLength;
	return (path->isInsidePath (position () - bbFront + bbSide) &&
			path->isInsidePath (position () + bbFront + bbSide) &&
			path->isInsidePath (position () + bbFront - bbSide) &&
			path->isInsidePath (position () - bbFront - bbSide));
}


OpenSteer::Vec3 AITraffic_Vehicle::convertAbsoluteToIncrementalSteering (const OpenSteer::Vec3& absolute,
                                               const float elapsedTime)
{
	const OpenSteer::Vec3 curved = convertLinearToCurvedSpaceGlobal (absolute);
	blendIntoAccumulator (elapsedTime * 8.0f, curved, currentSteering);
    return currentSteering;
}

OpenSteer::Vec3 AITraffic_Vehicle::convertLinearToCurvedSpaceGlobal (const OpenSteer::Vec3& linear)
{
	const OpenSteer::Vec3 trimmedLinear = linear.truncateLength (maxForce ());

	// ---------- this block imported from steerToAvoidObstaclesOnMap
	const float signedRadius = 1 / (nonZeroCurvatureQQQ()  * 1);
	const OpenSteer::Vec3 localCenterOfCurvature = side () * signedRadius;
	const OpenSteer::Vec3 center = position () + localCenterOfCurvature;
	const float sign = signedRadius < 0 ? 1.0f : -1.0f;
	const float arcLength = trimmedLinear.dot (forward ());
	//
	const float arcRadius = signedRadius * -sign;
	const float twoPi = 2 * OPENSTEER_M_PI;
	const float circumference = twoPi * arcRadius;
	const float arcAngle = twoPi * arcLength / circumference;
	// ---------- this block imported from steerToAvoidObstaclesOnMap

	// ---------- this block imported from scanObstacleMap
	// vector from center of curvature to position of vehicle
	const OpenSteer::Vec3 initialSpoke = position () - center;
	// rotate by signed arc angle
	const OpenSteer::Vec3 spoke = initialSpoke.rotateAboutGlobalY (arcAngle * sign);
	// ---------- this block imported from scanObstacleMap

	const OpenSteer::Vec3 fromCenter = -localCenterOfCurvature.normalize ();
	const float dRadius = trimmedLinear.dot (fromCenter);
	const float radiusChangeFactor = (dRadius + arcRadius) / arcRadius;
	const OpenSteer::Vec3 resultLocation = center + (spoke * radiusChangeFactor);

	// return the vector from vehicle position to the coimputed location
	// of the curved image of the original linear offset
	return resultLocation - position ();
}


    // approximate value for the Polaris Ranger 6x6: 16 feet, 5 meters
float AITraffic_Vehicle::minimumTurningRadius ()
{
	return 5.0f;
}


OpenSteer::Vec3 AITraffic_Vehicle::adjustSteeringForMinimumTurningRadius (const OpenSteer::Vec3& steering)
{
	return steering;//imike
	const float maxCurvature = 1 / (minimumTurningRadius () * 1.1f);

	// are we turning more sharply than the minimum turning radius?
	if (absXXX (curvature ()) > maxCurvature)
		{
            // remove the tangential (non-thrust) component of the steering
            // force, replace it with a force pointing away from the center
            // of curvature, causing us to "widen out" easing off from the
            // minimum turing radius
            const float signedRadius = 1 / nonZeroCurvatureQQQ ();
            const float sign = signedRadius < 0 ? 1.0f : -1.0f;
            const OpenSteer::Vec3 thrust = steering.parallelComponent (forward ());
            const OpenSteer::Vec3 trimmed = thrust.truncateLength (maxForce ());
            const OpenSteer::Vec3 widenOut = side () * maxForce () * sign;

			return trimmed + widenOut;
        }

	// otherwise just return unmodified input
	return steering;
}


// QQQ This is to work around the bug that scanObstacleMap's current
// QQQ arguments preclude the driving straight [curvature()==0] case.
// QQQ This routine returns the current vehicle path curvature, unless it
// QQQ is *very* close to zero, in which case a small positive number is
// QQQ returned (corresponding to a radius of 100,000 meters).  
// QQQ
// QQQ Presumably it would be better to get rid of this routine and
// QQQ redesign the arguments of scanObstacleMap
//
float AITraffic_Vehicle::nonZeroCurvatureQQQ (void)
{
	const float c = curvature ();
	const float minCurvature = 1.0f / 100000.0f; // 100,000 meter radius
	const bool tooSmall = (c < minCurvature) && (c > -minCurvature);
	return (tooSmall ? minCurvature: c);
}


// QQQ ad hoc speed limitation based on path orientation...
// QQQ should be renamed since it is based on more than curvature
float AITraffic_Vehicle::maxSpeedForCurvature ()
{
	float maxRelativeSpeed = 1;

	if (curvedSteering && false)
        {
            // compute an ad hoc "relative curvature"
            const float absC = absXXX (curvature ());
            const float maxC = 1 / minimumTurningRadius ();
            const float relativeCurvature = sqrtXXX (clip (absC/maxC, 0, 1));

            // map from full throttle when straight to 10% at max curvature
            const float curveSpeed = interpolate (relativeCurvature,1.0f,0.1f);
            annoteMaxRelSpeedCurve = curveSpeed;
            maxRelativeSpeed = curveSpeed;
        }
	annoteMaxRelSpeed = maxRelativeSpeed;
	return maxSpeed () * maxRelativeSpeed;
}


// xxx library candidate
// xxx assumes (but does not check or enforce) heading is unit length
OpenSteer::Vec3 AITraffic_Vehicle::steerTowardHeading (const OpenSteer::Vec3& desiredGlobalHeading)
{
	const OpenSteer::Vec3 headingError = desiredGlobalHeading - forward ();
	return headingError.normalize () * maxForce ();
}


//Direct transport from OS::SimpleVehicle


OpenSteer::Vec3 AITraffic_Vehicle::adjustRawSteeringForce (const Vec3& force,
                                                  const float )
{
 const float maxAdjustedSpeed = 0.2f * maxSpeed ();

    if ((speed () > maxAdjustedSpeed) || (force == Vec3::zero))
    {
        return force;
    }
    else
    {
        const float range = speed() / maxAdjustedSpeed;
        // const float cosine = interpolate (pow (range, 6), 1.0f, -1.0f);
        // const float cosine = interpolate (pow (range, 10), 1.0f, -1.0f);
        // const float cosine = interpolate (pow (range, 20), 1.0f, -1.0f);
        // const float cosine = interpolate (pow (range, 100), 1.0f, -1.0f);
        // const float cosine = interpolate (pow (range, 50), 1.0f, -1.0f);
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
// maybe the guts of applySteeringForce should be split off into a subroutine
// used by both applySteeringForce and applyBrakingForce?


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

	Ogre::LogManager::getSingleton().logMessage("STEERINGFORCE: "+Ogre::StringConverter::toString(force.x)+" "+Ogre::StringConverter::toString(force.y)+" "+Ogre::StringConverter::toString(force.z));
    const Vec3 adjustedForce = adjustRawSteeringForce (force, elapsedTime);
	Ogre::LogManager::getSingleton().logMessage("ADJUSTEDFORCE: "+Ogre::StringConverter::toString(adjustedForce.x)+" "+Ogre::StringConverter::toString(adjustedForce.y)+" "+Ogre::StringConverter::toString(adjustedForce.z));

    // enforce limit on magnitude of steering force
    const Vec3 clippedForce = adjustedForce.truncateLength (maxForce ());
	Ogre::LogManager::getSingleton().logMessage("CLIPPEDFORCE: "+Ogre::StringConverter::toString(clippedForce.x)+" "+Ogre::StringConverter::toString(clippedForce.y)+" "+Ogre::StringConverter::toString(clippedForce.z));

    // compute acceleration and velocity
//    Vec3 newAcceleration = (clippedForce / mass());
    Vec3 newAcceleration = (clippedForce);
	Ogre::LogManager::getSingleton().logMessage("NEWACCELERATION: "+Ogre::StringConverter::toString(newAcceleration.x)+" "+Ogre::StringConverter::toString(newAcceleration.y)+" "+Ogre::StringConverter::toString(newAcceleration.z));

    Vec3 newVelocity = velocity();
	Ogre::LogManager::getSingleton().logMessage("NEWVELOCITY-1: "+Ogre::StringConverter::toString(newVelocity.x)+" "+Ogre::StringConverter::toString(newVelocity.y)+" "+Ogre::StringConverter::toString(newVelocity.z));

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
	Ogre::LogManager::getSingleton().logMessage("NEWVELOCITY-2: "+Ogre::StringConverter::toString(newVelocity.x)+" "+Ogre::StringConverter::toString(newVelocity.y)+" "+Ogre::StringConverter::toString(newVelocity.z));

    // enforce speed limit
    newVelocity = newVelocity.truncateLength (maxSpeed ());

    // update Speed
    setSpeed (newVelocity.length());

    // Euler integrate (per frame) velocity into position
    static_cast<AbstractVehicle*>(this)->setPosition (position() + (newVelocity * elapsedTime));

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


void AITraffic_Vehicle::regenerateLocalSpace (const Vec3& newVelocity, const float )
{
    // adjust orthonormal basis vectors to be aligned with new velocity
    if (speed() > 0) regenerateOrthonormalBasisUF (newVelocity / speed());
}


// ----------------------------------------------------------------------------
// alternate version: keep FORWARD parallel to velocity, adjust UP according
// to a no-basis-in-reality "banking" behavior, something like what birds and
// airplanes do

// XXX experimental cwr 6-5-03


void AITraffic_Vehicle::regenerateLocalSpaceForBanking (const Vec3& newVelocity, const float elapsedTime)
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

//  annotationLine (position(), position() + (globalUp * 4), gWhite);  // XXX
//  annotationLine (position(), position() + (bankUp   * 4), gOrange); // XXX
//  annotationLine (position(), position() + (accelUp  * 4), gRed);    // XXX
//  annotationLine (position(), position() + (up ()    * 1), gYellow); // XXX

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
// draw lines from vehicle's position showing its velocity and acceleration


void AITraffic_Vehicle::annotationVelocityAcceleration (float maxLengthA, 
                                                          float maxLengthV)
{
    const float desat = 0.4f;
    const float aScale = maxLengthA / maxForce ();
    const float vScale = maxLengthV / maxSpeed ();
    const Vec3& p = position();
    const Vec3 aColor (desat, desat, 1); // bluish
    const Vec3 vColor (    1, desat, 1); // pinkish

}


// ----------------------------------------------------------------------------
// predict position of this vehicle at some time in the future
// (assumes velocity remains constant, hence path is a straight line)
//
// XXX Want to encapsulate this since eventually I want to investigate
// XXX non-linear predictors.  Maybe predictFutureLocalSpace ?
//
// XXX move to a vehicle utility mixin?


OpenSteer::Vec3 AITraffic_Vehicle::predictFuturePosition (const float predictionTime) const
{
    return position() + (velocity() * predictionTime);
}


// ----------------------------------------------------------------------------
*/



// -------------------------------- SIMPLE AI TRAFFIC DRIVE ------------------------------------

void AITraffic_Vehicle::updateSimple(const float currentTime, const float elapsedTime)
{
	// are we in the path-tube?
	// if so find the neares one
	// for mow we assume: yes

	if (aimatrix->lane(ps_idx, wp_prev_idx, wp_idx, getPosition())==-1)
		{
			// out of path
		}

	// are we close to the next waypoint
	// if so,update the target, means we set the 

	if (closeToWayPoint(getHeadedWayPoint(),5.0f))
		{
			advanceToNextWayPoint();
		}

	int safe_follow		= calculateSafeFollowDistance();
	int brake_dist		= calculateBrakeDistance();
	int safe_distance	= objectsOnTravelPath();
	advance(currentTime);
}

int	 AITraffic_Vehicle::closestWayPoint()
{	
	int idx = 0;
	float dist = position.distance(aimatrix->trafficgrid->waypoints[0].position);
	for (int i=1;i<aimatrix->trafficgrid->num_of_waypoints;i++)
		{
			float d2 = position.distance(aimatrix->trafficgrid->waypoints[i].position);
			if (d2<dist) { dist = d2; idx = i; }
		}
	return idx;
}

float AITraffic_Vehicle::calculateSafeFollowDistance()
{
	return 10.0;
}

float  AITraffic_Vehicle::calculateBrakeDistance()
{
	return 5.0;
}

int	AITraffic_Vehicle::getHeadedWayPoint()
{
	return wp_idx;
}

int	AITraffic_Vehicle::getLeftWayPoint()
{
	return wp_prev_idx;
}

int	AITraffic_Vehicle::advanceToNextWayPoint()
{
	// we create separate function for this, since we will implement "direction" of travel later on by these
	ps_idx += path_direction;
	if (ps_idx>=aimatrix->trafficgrid->paths[path_id].num_of_segments || ps_idx<0) 
		{
			int tmp = ps_idx;
			switch(aimatrix->trafficgrid->paths[path_id].path_type)
				{
					case 0:
						active = false;		// this vehicle does not go anywhere anymore
						break;
					case 1:
						ps_idx		= 0;
						setPosition(aimatrix->trafficgrid->waypoints[aimatrix->trafficgrid->paths[path_id].segments[ps_idx]].position);
						break;
					case 2:	// not implemented yet
						path_direction = -path_direction;
						ps_idx += path_direction;
						break;
				}
		}

	int cseg = aimatrix->trafficgrid->paths[path_id].segments[ps_idx]; // current segment index
	if (path_direction>0)	// forward on path
		{
			wp_prev_idx = aimatrix->trafficgrid->segments[cseg].start;
			wp_idx		= aimatrix->trafficgrid->segments[cseg].end;
		}
	else					// backward on path
		{
			wp_prev_idx = aimatrix->trafficgrid->segments[cseg].end;
			wp_idx		= aimatrix->trafficgrid->segments[cseg].start;
		}

	Ogre::Vector3 new_wp = aimatrix->trafficgrid->waypoints[wp_idx].position;
//	new_wp = aimatrix->trafficgrid

	forward = new_wp-getPosition();
	forward.normalise();
	
	return wp_idx;
}


bool  AITraffic_Vehicle::closeToWayPoint(int idx, float r)
{
	bool retbool = false;
	if (position.distance(aimatrix->trafficgrid->waypoints[idx].position)<=r) retbool = true;
	return retbool;
}

void  AITraffic_Vehicle::advance(float deltat)
{
	position += forward * deltat * speed;
}

// this is the main function to identify all related objects in our path

float  AITraffic_Vehicle::objectsOnTravelPath()
{
	float r = 30.0f;		// distance we want sweep within
	int nums = aimatrix->num_of_objs;
	const Ogre::Vector3		pos = getPosition();
	const Ogre::Quaternion	ori = getOrientation();

	int shield[36];
	for (int i=0;i<36;i++) shield[i] = 0;

//	for (int i=0;i<nums;i++)	// not efficient if too many objects, prefilter should be used here
	for (int i=0;i<=3;i++)	// not efficient if too many objects, prefilter should be used here
	{
		if (i!=serial)
			{
				const Ogre::Vector3 target = aimatrix->trafficgrid->trafficnodes[i].position;

				if (pos.distance(target)<r)	// object within distance
					{
						Ogre::Vector3 target_diff = target-pos;
						Ogre::Quaternion sq = forward.getRotationTo(target_diff,Ogre::Vector3::UNIT_Y);
						sq.normalise();
						float yaw = sq.getYaw().valueDegrees()+180.0f;
						int idx = (int) yaw/10.0f;
						if (idx<0) idx = 0;
						else if (idx>35) idx = 35;

						shield[idx]++;
					}
			}
	}

	return 0.0f;
	// we have shield info
	// now let's calculate where to steer or speed next 

	// check for -30..30 degrees ahead for object
	bool obs = false;

	for (int i=17;i<19;i++)
		{
			if (shield[i]) obs = true;
		}

	if (obs) speed -= 5.0;
	else speed+=0.1;
	if (speed>50) speed = 50;
	if (speed<0) speed = 0;
	return 100.0f;
}

Ogre::Vector3 AITraffic_Vehicle::getPosition()
{
//	OpenSteer::AbstractVehicle* vehicle = this;
//	return Ogre::Vector3(vehicle->position().x,vehicle->position().y,vehicle->position().z);
	return position;
}

void AITraffic_Vehicle::setPosition(Ogre::Vector3 newPos)
{
//	static_cast<AbstractVehicle*>(this)->setPosition(OpenSteer::Vec3(newPos.x, newPos.y, newPos.z));
	position = newPos;
}

Ogre::Quaternion AITraffic_Vehicle::getOrientation()
{
	Ogre::Vector3 v1(-1, 0, 0);
	Ogre::Vector3 v2(forward.x, forward.y, forward.z);
	Ogre::Quaternion retquat = v1.getRotationTo(v2, Ogre::Vector3::UNIT_Y);
	return retquat;
}


/*
void AITraffic_Vehicle::makePath (void)
{
		num_of_waypoints = 4;
		waypoints[0] = Ogre::Vector3(30.2037,		0,	15.4732);
		waypoints[1] = Ogre::Vector3(144.78,		0,	14.9248);
		waypoints[2] = Ogre::Vector3(144.34,		0,	89.4973);
		waypoints[3] = Ogre::Vector3(15.1772,		0,	90.9755);

        // return Path object
        const int pathPointCount = 4;
        const float k = 2.05f;
        float pathRadii[4];
		for (int i=0;i<pathPointCount;i++) pathRadii[i] = k;
        return new AITraffic_Route (pathPointCount, waypoints, pathRadii, true);

}
*/

#endif //OPENSTEER
