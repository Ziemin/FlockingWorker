
#include "targetver.h"

#include <stdio.h>

#include <ctime>
#include <iostream>
#include <string>
#include <thread>
#include <list>
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>

#include <improbable/worker.h>

#include "Maths.h"

#include "demoteam/flock.h"
#include "demoteam/transform.h"
#include <atomic>

#include "flocking.h"
#include "Geometry.h"

#define USE_PARTITIONING
//#define DEBUG_PARTITIONING

//#pragma optimize("", off)

using namespace improbable::math;
using namespace demoteam;
using namespace geometry;

namespace
{
	const std::string kWorkerType = "FlockingWorker";
	const int g_targetFPS = 8;
	const float g_secondsPerFrame = 1.0f / g_targetFPS;
	const long long g_millisecondsPerFrame = 1000LL * g_secondsPerFrame;
	const long long g_millisecondsBetweenMetrics = 1000LL;
	const int g_maxNeighbours = 32;
	const int g_numThreads = 8;

	enum ExecutionState
	{
		NotRunning = 0,
		Running,
		Quitting
	};
	std::atomic_int g_ExecutionState(NotRunning);
	
	typedef std::pair<worker::EntityId, int> TEntityStorage;
	typedef std::list<TEntityStorage> TEntities;

	//------------------------------------------
	struct SpatialBucket
	{
		SpatialBucket(int gridIdx, const Aabb3& box, worker::EntityId entId, int entIdx) : Box(box), GridIndex(gridIdx), IntersectionHelper(box, 18.0f)
		{
			Entities.push_back(std::make_pair(entId, entIdx));
		}
		TEntities Entities;
		Aabb3 Box;
		CubeSphereIntersection IntersectionHelper;
		int GridIndex;
	};
	typedef std::vector<std::shared_ptr<SpatialBucket> > TBuckets;
	
	//------------------------------------------
	struct NeighbourData
	{
		NeighbourData() : Transform(zero3<Coordinates>(), unitZ3<Vector3f>(), zero3<Vector3f>()) {}
		NeighbourData(const worker::EntityId& entityId, const TransformData& tr, float distSqr) : EntityId(entityId), Transform(tr), DistanceSqr(distSqr) {}
		worker::EntityId EntityId;
		TransformData Transform;
		float DistanceSqr;
	};
	
	//***************************************************************************************************************
	int GetFurthestNeighbour(const NeighbourData* closestBuffer, int nclosest)
	{
		int ifurthest = 0;

		for (int c0 = 1; c0 < nclosest; ++c0)
		{
			ifurthest = closestBuffer[c0].DistanceSqr > closestBuffer[ifurthest].DistanceSqr ? c0 : ifurthest;
		}

		return ifurthest;
	}

	typedef std::vector<worker::EntityId> TFlockers;
	typedef std::vector<const TransformData*> TTransformCache;

	struct SUpdateUpdate
	{
		SUpdateUpdate() : pos(Coordinates(0, 0, 0)), facing(0, 0, 1), velocity(0, 0, 0) {}
		Coordinates pos;
		Vector3f facing;
		Vector3f velocity;
	};
	typedef std::vector<worker::EntityId> TFlockers;
	typedef std::vector<SUpdateUpdate> TFlockersUpdate;
}

//***************************************************************************************************************
Vector3f CalculateSteeringVector(
	const TransformData& transform, 
	const FlockingData& params, 
	const NeighbourData* closestBuffer, 
	int nclosest)
{
	Vector3f averagePos = zero3<Vector3f>();
	Vector3f averageVel = zero3<Vector3f>();
	Vector3f deltaSepSum = zero3<Vector3f>();

	float oneOnN = nclosest>0 ? (1.0f / nclosest) : 0.0f;

	auto calculateDeltaSep = [params](TVector3fArg lineAway)
	{
		float separationK = ln2 / sqr(params.repel_separation_for_half());
		float mag = expf(-separationK*sqrMag(lineAway));
		return normalize(lineAway)*mag;
	};

	for (int c0 = 0; c0 < nclosest; ++c0)
	{
		auto dat = closestBuffer[c0];
		auto neighbourTransform = dat.Transform;
		
		averagePos = averagePos + toVector3f(neighbourTransform.position())*oneOnN;
		averageVel = averageVel + neighbourTransform.velocity()*oneOnN;
		
		Vector3f lineAway = transform.position() - neighbourTransform.position();
		auto deltaSep = sqrMag(lineAway) > epsilon ? 
			calculateDeltaSep(lineAway) : 
			zero3<Vector3f>();

		deltaSepSum = deltaSepSum + deltaSep;
	}

	auto deltaPos = (averagePos - toVector3f(transform.position()));
	auto deltaVel = (averageVel - transform.velocity());

	return	deltaPos*params.attract_coefficient() +
			deltaVel*params.follow_coefficient() +
			deltaSepSum*params.repel_coefficient();
}

//***************************************************************************************************************
TVector3fRet KeepAtGoodHeight(const TransformData& transform, TVector3fArg steeringVector)
{
	auto height = dot(toVector3f(transform.position()), unitY3<Vector3f>());
	
	auto shouldInvertY = [height, steeringVector]()
	{
		const float minHeight = 10.0f;
		const float maxHeight = 30.0f;
		return ((height<minHeight && steeringVector.Y() < 0.0f) || (height > maxHeight && steeringVector.Y()>0.0f));
	};
	auto invertY = [](TVector3fArg steeringVector)
	{
		return steeringVector - 2 * steeringVector*unitY3<Vector3f>();
	};

	return shouldInvertY() ? invertY(steeringVector) : steeringVector;

}

//***************************************************************************************************************
TVector3fRet KeepNearOrigin(const TransformData& transform, TVector3fArg steeringVector)
{
	const float maxDistance = 192.0f;
	auto toOrigin = zero3<Vector3f>() - toVector3f(transform.position());
	auto sqrDist = sqrMag(toOrigin);
	return steeringVector + (sqrDist > epsilon ? normalize(toOrigin)*powf(sqrDist / sqr(maxDistance), 16.0f) : zero3<Vector3f>());
}

namespace
{
	const unsigned int maxBitsX = 11;
	const unsigned int maxBitsY = 10;
	const unsigned int maxBitsZ = 11;
}

unsigned int calcGridIndex(TVector3fArg pos, const Aabb3& worldExtents, float gridSize)
{
//	assert(boxContains(pos));

	auto maxIndices = (worldExtents.RightTopFront - worldExtents.LeftBottomBack)/gridSize;

	//assert(maxIndices.X() < (1 << maxBitsX));
	//assert(maxIndices.Y() < (1 << maxBitsY));
	//assert(maxIndices.Z() < (1 << maxBitsZ));

	Vector3f gridIndices = (pos - worldExtents.LeftBottomBack)/gridSize;

	unsigned int idX = gridIndices.X();
	unsigned int idY = gridIndices.Y();
	unsigned int idZ = gridIndices.Z();

	return idX + (idY << maxBitsX) + (idZ << (maxBitsX + maxBitsY));
}

Aabb3 gridIndexToBox(unsigned int gridIndex, const Aabb3& worldExtents, float gridSize)
{
	unsigned int maskX = (1 << maxBitsX) - 1;
	unsigned int maskY = (1 << maxBitsY) - 1;
	unsigned int maskZ = (1 << maxBitsZ) - 1;
	unsigned int offsetX = 0;
	unsigned int offsetY = offsetX+maxBitsX;
	unsigned int offsetZ = offsetY+maxBitsY;
	unsigned int idX = (gridIndex >> offsetX)&maskX;
	unsigned int idY = (gridIndex >> offsetY)&maskY;
	unsigned int idZ = (gridIndex >> offsetZ)&maskZ;

	return Aabb3(worldExtents.LeftBottomBack + Vector3f(idX, idY, idZ)*gridSize, worldExtents.LeftBottomBack + Vector3f(idX + 1, idY + 1, idZ + 1)*gridSize);
}

void BuildSpatialGrid(TBuckets& buckets, const worker::View& view)
{
	float len = 1000.0f;
	Aabb3 worldExtents(Vector3f(-len,-len,-len), Vector3f(len, len, len));
	float gridSize = 8.0f;

	auto itBegin = view.Entities.begin();
	auto itEnd = view.Entities.end();

	int nent = 0;
	
	// spatial partitioning test
	for (auto itEnt = itBegin; itEnt != itEnd; ++itEnt)
	{
		auto transform = itEnt->second.Get<Transform>();
		auto gridIdx = calcGridIndex(toVector3f(transform->position()), worldExtents, gridSize);

		auto itBuck = std::find_if(buckets.begin(), buckets.end(), [gridIdx](const std::shared_ptr<SpatialBucket>& bucket) { return bucket->GridIndex == gridIdx;  });
		if (itBuck == buckets.end())
		{
			// make a new one
			auto box = gridIndexToBox(gridIdx, worldExtents, gridSize);
			//assert(boxContains(box, transform->position()));
			if (!boxContains(box, toVector3f(transform->position())))
			{
				printf("placed entity in a box that does not well describe its position :(\n");
			}
			buckets.push_back(std::shared_ptr<SpatialBucket>(new SpatialBucket(gridIdx, box, itEnt->first, nent)));
		}
		else
		{
			auto& bucketPtr = *itBuck;
			bucketPtr->Entities.push_back(std::make_pair(itEnt->first, nent));
		}
		++nent;
	}	
}

void forAllEntitiesWithinRadius(const TBuckets& spatialGrid, const TransformData*const* transformCache, const worker::View& view, const Sphere& sphere, std::function<void(worker::EntityId, TransformData)> func)
{
	int nentsTotal = 0;
	int nentsBucket = 0;
	int nentsRange = 0;

	for (auto itGrid = spatialGrid.begin(); itGrid != spatialGrid.end(); ++itGrid)
	{
		auto& buck = *itGrid;
		int nentsThisBucket = buck->Entities.size();
		nentsTotal += nentsThisBucket;

		//if (boxSphereOverlap(buck->Box, sphere))
		if(buck->IntersectionHelper.IntersectionAt(sphere.Origin))
		{
			nentsBucket += nentsThisBucket;

			auto itEntEnd = buck->Entities.end();
			for (auto itEnt = buck->Entities.begin(); itEnt != itEntEnd; ++itEnt)
			{
				auto& ent = *itEnt;
				auto transform = transformCache[ent.second];

				if (transform != nullptr && sphereContains(sphere, toVector3f(transform->position())))
				{
					++nentsRange;
					func(ent.first, *transform);
				}
			}
		}
	}

	//printf("SpatialGrid: %d|%d|%d\n", nentsTotal, nentsBucket, nentsRange);
}
//***************************************************************************************************************
int CountEntitiesWithinLinearSearch(const worker::View& view, const TransformData*const* transformCache, const worker::EntityId& flockerId, TVector3fArg pos, float r)
{
	auto itBegin = view.Entities.begin();
	auto itEnd = view.Entities.end();
	int nentitiesLinear = 0;

	int ient = -1;
	for (auto itEnt = itBegin;
	itEnt != itEnd;
		++itEnt)
	{
		auto neighbourTransform = transformCache[++ient]; //itFlocker->second.Get<Transform>();
		if (neighbourTransform != nullptr && itEnt->first != flockerId && sqrMag(toVector3f(neighbourTransform->position()) - pos) < sqr(r))
		{
			++nentitiesLinear;
		}
	}

	return nentitiesLinear;
}

//***************************************************************************************************************
void UpdateFlocking(
	TFlockers& flockers, 
	TFlockersUpdate& flockersUpdate,
	const TTransformCache& transformCacheStorage,
	const TBuckets& spatialGrid,
	int ibegin,
	int iend,
	const worker::View& view, 
	worker::Connection& connection, 
	const float timeStep)
{
	auto updateComponent = [&connection, &view, timeStep](
			const worker::EntityId& entityId, 
			const worker::Option<TransformData>& transform, 
			const worker::Option<FlockingData>& params,
			const NeighbourData* closestNeighbours,
			int numClosest,
			SUpdateUpdate& targetUpdate
		)
	{
		Vector3f steeringVector = CalculateSteeringVector(	*transform,
															*params, 
															closestNeighbours, 
															numClosest);

		steeringVector = KeepNearOrigin(*transform, steeringVector);
		steeringVector = KeepAtGoodHeight(*transform, steeringVector);
									
		// rotate forward
		auto newFwd = transform->forward();
		if (sqrMag(steeringVector) > epsilon)
		{	
			const float maxAngle = toRadians(params->max_turn_degrees_per_second());

			auto targetFacing = normalize(steeringVector);
			auto cosAng = dot(newFwd, targetFacing);
			auto ang = acosf(std::max(std::min(cosAng, 1.0f), -1.0f));
			auto ey = cross(newFwd, targetFacing);
			if (!isZero(ey, epsilon))
			{
				auto ez = newFwd;
				auto ex = cross(normalize(ey), ez);
				auto angLimited = std::min(ang, maxAngle);
				newFwd = normalize(ez*cosf(angLimited) + ex*sinf(angLimited));
			}
		}

		Vector3f newVel = newFwd*params->speed();
		auto newPos = transform->position() + newVel*timeStep;
		
		targetUpdate.pos = newPos;
		targetUpdate.facing = newFwd;
		targetUpdate.velocity = newVel;
	};
			
	auto itBegin = view.Entities.begin();
	auto itEnd = view.Entities.end();
	
	NeighbourData closestNeighboursStore[g_maxNeighbours];
	NeighbourData* closestNeighbours = closestNeighboursStore;

	const TransformData*const * transformCache = &transformCacheStorage[0];

	int nNeighbours;
	int ifurthest = 0;

	int niters = 0;

	for (int idelegate = ibegin; idelegate < iend; ++idelegate)
	{
		auto flockerId = flockers[idelegate];
		auto itEnt = view.Entities.find(flockerId);
		if (itEnt == itEnd)
		{
			connection.SendLogMessage(worker::LogLevel::WARN, "flockingWorker", "delegation for unknown entity [" + std::to_string(flockerId) + "]");
			continue;
		}
		
		int nitersLocal = 0;

		auto ent = itEnt->second;
		auto& paramsOption = ent.Get<Flock>();
		auto& transformOption = ent.Get<Transform>();
		nNeighbours = 0;
		if (!transformOption.empty() && !paramsOption.empty())
		{
			const FlockingData& params = *paramsOption;
			const TransformData& transform = *transformOption;

			auto sqrDist = [transform](const TransformData& neighbourTransform) {
				return sqrMag(neighbourTransform.position() - transform.position());
			};
			auto writeClosestNeighbours = [transform, params, &nNeighbours, closestNeighbours, &ifurthest, sqrDist](const worker::EntityId& neighbourId, const TransformData& neighbourTransform) {
				if (ShouldConsiderEntity(transform,
					neighbourTransform,
					params.search_range()))
				{
					if (nNeighbours < params.number_to_consider())
					{
						int idx = nNeighbours++;

						closestNeighbours[idx] =
							NeighbourData(neighbourId, neighbourTransform, sqrDist(neighbourTransform));

						ifurthest =
							closestNeighbours[idx].DistanceSqr >
							closestNeighbours[ifurthest].DistanceSqr ?
							idx :
							ifurthest;
					}
					else if (sqrDist(neighbourTransform)<closestNeighbours[ifurthest].DistanceSqr)
					{
						closestNeighbours[ifurthest] =
							NeighbourData(neighbourId, neighbourTransform, sqrDist(neighbourTransform));

						// recalculate the furthest
						ifurthest = GetFurthestNeighbour(closestNeighbours, nNeighbours);
					}
				}
			};
#ifdef USE_PARTITIONING

#ifdef DEBUG_PARTITIONING
			int nentitiesLinear = CountEntitiesWithinLinearSearch(view, transformCache, flockerId, toVector3f(transform.position()), params.search_range());
#endif //DEBUG_PARTITIONING
			
			Sphere sphere = { toVector3f(transform.position()), params.search_range() };
			forAllEntitiesWithinRadius(spatialGrid, transformCache, view, sphere, [flockerId, &nitersLocal, &writeClosestNeighbours](const worker::EntityId& neighbourId, const TransformData& neighbourTransform)
			{
				if (neighbourId != flockerId)
				{
					++nitersLocal;
					writeClosestNeighbours(neighbourId, neighbourTransform);
				}
			});

#ifdef DEBUG_PARTITIONING
			assert(nentitiesLinear == nitersLocal);
			if (nentitiesLinear != nitersLocal)
			{
				printf("disparity! Spatial: %d; Actual %d\n", nitersLocal, nentitiesLinear);
			}
#endif //DEBUG_PARTITIONING
#else
			int ient = -1;
			for (	auto itEnt = itBegin;
					itEnt != itEnd;
					++itEnt)
			{
				auto neighbourTransform = transformCache[++ient]; //itFlocker->second.Get<Transform>();
				if (neighbourTransform!=nullptr && itEnt->first != flockerId)
				{
					++nitersLocal;
					writeClosestNeighbours(itEnt->first, *neighbourTransform);
				}
			}
#endif //USE_PARTITIONING

			niters += nitersLocal;

			updateComponent(flockerId, transform, params, closestNeighbours, nNeighbours, flockersUpdate[idelegate]);
		}
	}
}

//***************************************************************************************************************
void Run(worker::Connection& connection)
{ 
	TFlockers flockers;
	TFlockersUpdate flockersUpdate;

	flockersUpdate.resize(2048);
	worker::View view;
	view.OnAuthorityChange<Transform>(
		[&flockers, &flockersUpdate](const worker::AuthorityChangeOp& op)
		{
			if (op.HasAuthority)
			{
				auto itFind = std::find(flockers.begin(), flockers.end(), op.EntityId);
				if (itFind == flockers.end())
				{
					flockers.push_back(op.EntityId);
				}
			}
			else
			{
				flockers.erase(std::remove(flockers.begin(), flockers.end(), op.EntityId), flockers.end());
			}

			if (flockers.size() > flockersUpdate.size())
			{
				flockersUpdate.resize(flockers.size());
			}
		}
	);
	
	view.OnRemoveEntity([&flockers](const worker::RemoveEntityOp& op)
		{
			flockers.erase(std::remove(flockers.begin(), flockers.end(), op.EntityId), flockers.end());
		}
	);
	
	auto nextUpdate = std::chrono::system_clock::now();
	auto nextMetrics = nextUpdate;
	
	const int maxLoadBufEntries = 16;
	float loadBuf[maxLoadBufEntries];
	for (auto cbuf = 0; cbuf < maxLoadBufEntries; ++cbuf)
		loadBuf[cbuf] = 0.0f;
	int loadBufHead = maxLoadBufEntries -1;

	auto calcAverageLoad = [&loadBuf, maxLoadBufEntries]() 
	{
		auto sumLoad = 0.0f;
		for (auto ibuf = 0; ibuf < maxLoadBufEntries; ++ibuf)
		{
			sumLoad += loadBuf[ibuf];
		}
		return sumLoad / maxLoadBufEntries;
	};

	// flocking thread pool
	const int numThreadsLocal = g_numThreads;
	const int allFlags = (1 << g_numThreads) - 1;
	std::thread threads[g_numThreads];

	std::atomic_int workStatus(0);
	float loadStore = 1.0f;

	TTransformCache transformCache;
	TBuckets spatialGrid;
	
	// initialise the worker thread pool
	for (int c0 = 0; c0 < g_numThreads; ++c0)
	{
		threads[c0] = std::thread([&flockers, &flockersUpdate, &transformCache, &spatialGrid, &view, &connection, &loadStore, c0, numThreadsLocal, allFlags, &workStatus]() {

			int threadId = c0;

			while (g_ExecutionState.fetch_and(Running) == Running)
			{
				// wait for work
				int flag = 1 << threadId;
				if ((workStatus.load()&flag)!=0)
				{
					// choose what to take based on the threadId and the number of flockers
					int nflockers = flockers.size();
					if (nflockers < numThreadsLocal)
					{
						// do them all
						if (threadId == 0)
						{
							UpdateFlocking(flockers, flockersUpdate, transformCache, spatialGrid, 0, nflockers, view, connection, g_secondsPerFrame*loadStore);
						}
					}
					else
					{
						int ndiv = nflockers/ numThreadsLocal;
						int ntakeTotal = ndiv*numThreadsLocal;
						int ntakeDiff = nflockers - ntakeTotal;

						int ibegin = threadId*ndiv;
						int ntake = ndiv + ((threadId == (numThreadsLocal - 1)) ? ntakeDiff : 0);

						UpdateFlocking(flockers, flockersUpdate, transformCache, spatialGrid, ibegin, ibegin+ ntake, view, connection, g_secondsPerFrame*loadStore);
					}					

					int expected;
					int newFlag;
					do
					{
						std::this_thread::yield();
						expected = workStatus.load();
						newFlag = expected & (~flag);
					} 
					while (!workStatus.compare_exchange_strong(expected, newFlag));

				}
				else
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
				}
			}

		});
	}
	
	while (g_ExecutionState.fetch_and(Running)==Running)
	{	
		auto theTimeNow = std::chrono::system_clock::now();
		if (theTimeNow > nextUpdate)
		{
			connection.SendLogMessage(worker::LogLevel::INFO, "FlockingWorker", "frame update");

			auto ops = connection.GetOpList(0, 0);
			view.Process(ops);

			{				
				// single thread
				//
				//UpdateFlocking(flockers, 0, flockers.size(), view, connection, g_secondsPerFrame*loadStore);
				
				// make sure we write to the load store
				loadStore = std::max(calcAverageLoad(), 1.0f); // make sure we don't go slower than optimum!

				// precache the transforms because Entity::Get<Transform> is crazy expensize!
				transformCache.resize(view.Entities.size(), nullptr);
				auto itEnd = view.Entities.end();
				int ient = -1;
				for (auto itEnt = view.Entities.begin(); itEnt != itEnd; ++itEnt)
				{
					auto& transformOption = itEnt->second.Get<Transform>(); // expensive!
					transformCache[++ient] = &*transformOption;
				}

				{
#ifdef USE_PARTITIONING
					spatialGrid.clear();
					BuildSpatialGrid(spatialGrid, view);		
#endif // 
				}

				// tell threads: It's time.
				workStatus.store(allFlags);

				// wait for threads
				while (workStatus.load() != 0)
				{
					std::this_thread::yield();
				}

				// force through the updates
				for (int iflock = 0; iflock < flockers.size(); ++iflock)
				{
					auto entId = flockers[iflock];
					auto flockUp = flockersUpdate[iflock];
						
					Transform::Update updTransform;
					updTransform.set_position(flockUp.pos);
					updTransform.set_velocity(flockUp.velocity);
					updTransform.set_forward(flockUp.facing);
					connection.SendComponentUpdate<Transform>(entId, updTransform);
				}
			}

			auto timeElapsed = std::chrono::system_clock::now() - theTimeNow;
			auto millisElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(timeElapsed).count();

			auto load = millisElapsed * 1.0f / g_millisecondsPerFrame;

			loadBufHead = (loadBufHead + 1) % maxLoadBufEntries;
			loadBuf[loadBufHead] = load;
				
			if (theTimeNow > nextMetrics)
			{
				worker::Metrics metrics;
				metrics.Load = calcAverageLoad();
				connection.SendMetrics(metrics);
				nextMetrics = theTimeNow + std::chrono::milliseconds(g_millisecondsBetweenMetrics);
			}

			auto millisRemaining = std::max(g_millisecondsPerFrame - millisElapsed, 0LL);
			nextUpdate = theTimeNow + std::chrono::milliseconds(millisRemaining);

			if (millisRemaining > 0)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(millisRemaining));
			}
			else
			{
				std::this_thread::yield();
			}
		}
		else
		{
			std::this_thread::yield();
		}
	}

	g_ExecutionState.store(Quitting);
}

int main(int argc, char**argv)
{
	unitTest();

	const char* ipAddress = argv[1];
	const int port = atoi(argv[2]);
	const char* workerId = argv[3]; 

	worker::ConnectionParameters wcp;
	wcp.WorkerType = "FlockingWorker";
	wcp.WorkerId = workerId;

	worker::Connection connection(
		ipAddress,
		port,
		wcp);

	g_ExecutionState.store(Running);
	
	Run(connection);
	
	while (g_ExecutionState.fetch_and(Running)==Running)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	g_ExecutionState.store(NotRunning);

	while (g_ExecutionState.fetch_and(Quitting) != Quitting)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}
	
	//thread.detach();

    return 0;
}

