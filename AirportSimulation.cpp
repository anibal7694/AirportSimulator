// AirportSimulation.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <mutex>
#include <unordered_map>
#include <queue>

using namespace std;
using namespace std::chrono;

/**
* @defgroup Boilerplate
* Basic support for logging and random sleeps. Nothing exciting to see here.
* @{ */

/** Some simple thread-safe logging functionality. */
void Log() {}

static recursive_mutex s_mutex;

template<typename First, typename ...Rest>
void Log(First&& first, Rest&& ...rest) {
	lock_guard<recursive_mutex> lock(s_mutex);
	std::cout << std::forward<First>(first);
	Log(std::forward<Rest>(rest)...);
}

/** Produces a random integer within the [1..max] range. */
static int RandomInt(int maxSleep = 6) {
	const int minSleep = 1;
	return minSleep + (rand() % static_cast<int>(maxSleep - minSleep + 1));
}

/** @} */

/** This can be used to initialize the airport simulation object operation duration parameter
* with a value which is used by the testing code. */
static constexpr const int kOperationDurationSec = 3;
enum class runwayState_
{
	Available, InOperation, Reserved,
};
enum class parkingStandState_
{
	Occupied, Reserved, Available,
};
/** A runway, essential for air travel. */
class Runway {
	/* TODO: implement :-) */
	//create a map that will have the set of runways and their status. 
	int length_;
	int runwayID_;

	runwayState_ objectState_;
public:
	Runway(const int &id, int len = 10) 
	{	
		length_ = len;
		runwayID_ = id;
		objectState_ = runwayState_::Available;
	}

	int GetRunwayID() 
	{
		return runwayID_;
	}
	void SetState(runwayState_ set)
	{
		objectState_ = set;
	}
	runwayState_ GetState()
	{
		return objectState_;
	}

};

/** Parking stand. Useful whether you're in a 747-800 or a station wagon. */
class ParkingStand {
	/* TODO: implement :-) */
	//create a map that will have the set of parking stands and their status. 
	int parkingStandID_;
	parkingStandState_ objectState_;
public: 

	ParkingStand(const int &id)
	{
		parkingStandID_ = id;
		objectState_ = parkingStandState_::Available;
	}
	int GetParkingID()
	{
		return parkingStandID_;
	}
	void SetState(parkingStandState_ set)
	{
		objectState_ = set;
	}
	parkingStandState_ GetState()
	{
		return objectState_;
	}
};

//using a separate status class because the landing tokens and take off tokens are for the same set of runways and parking stands
//and hence the resources are shared
class Status
{
	bool status_, used_;
	int aircraftID_;
	int reservedTime_;
	system_clock::time_point timeStamp_;
public:
	Status(const bool &stat) //this will run if there is only one argument (error case)
	{
		status_ = stat;
	}
	Status(const bool &stat, const int &id, const int &timeToExpire)
	{
		aircraftID_ = id;
		status_ = stat;
		reservedTime_ = timeToExpire;
		timeStamp_ = system_clock::now();
	}
	int GetAircraftID()
	{
		return aircraftID_;
	}
	int GetExpirationTime()
	{
		return reservedTime_;
	}
	void SetUsed()
	{
		used_ = true;
	}
	bool GetUsed()
	{
		return used_;
	}
	bool GetStatus()
	{
		return status_;
	}
	//checks if the reserved time is over, and will be used to reallocate the space 
	bool CheckValidity()
	{
		duration<double> timer = system_clock::now() - this->timeStamp_;
		if (timer.count() > this->reservedTime_)
			return false;
		else
			return true;		
	}
};
class LandingStatus: public Status {
	bool duplicate_;  
	unique_ptr<Runway> runwayPtr_;
	unique_ptr<ParkingStand> parkingPtr_;
public:
	LandingStatus(const bool &stat, bool dup = false) : Status{ stat }  //this will run if there are two arguments (error case)
	{
		duplicate_ = dup;
	}
	LandingStatus(const bool &stat, const int &id, unique_ptr<Runway> runwayID, unique_ptr<ParkingStand> parkingID, const int &timeToExpire) 
		: Status{ stat, id, timeToExpire }
	{
		runwayPtr_ = move(runwayID);
		parkingPtr_ = move(parkingID);
	}
	int RunwayID()
	{
		return runwayPtr_->GetRunwayID();
	}
	int ParkingStandID()
	{
		return parkingPtr_->GetParkingID();
	}
	bool Duplicate() 
	{
		return duplicate_;
	}
	unique_ptr<Runway> RunwayDetails()
	{
		return move(runwayPtr_);
	}
	unique_ptr<ParkingStand> ParkingStandDetails()
	{
		return move(parkingPtr_);
	}

};
class TakeoffStatus: public Status {
	unique_ptr<Runway> runway_;
public:
	TakeoffStatus(const bool &stat) : Status{ stat } {}  //this will run if there is only one argument (error case)
	TakeoffStatus(const bool &stat, const int &id, unique_ptr<Runway> runwayID, const int &timeToExpire) //now this will run if the airport id and runway id were available. 
		: Status{ stat, id, timeToExpire }
	{
		runway_ = move(runwayID);
	}
	int RunwayID()
	{
		return runway_->GetRunwayID();
	}
	unique_ptr<Runway> RunwayDetails()
	{
		return move(runway_);
	}
};
/** Simulation of an airport. Tiny preview of the headaches that come with the real thing. */
class Airport {
	/* TODO: implement :-) */
	unordered_map<int, int> aircraftDetails_;
	unordered_map<int, unique_ptr<ParkingStand>> parkingStandDetails_;

	queue<unique_ptr<Runway>> runwayQueue_;
	queue<unique_ptr<ParkingStand>> parkingQueue_;

	mutex muteResource_; 

public: 
	Airport(int runwayNum, int parkingNum, int OperationTime = 5)
	{
		aircraftDetails_.clear();
		parkingStandDetails_.clear();
		for (int i = 0; i < runwayNum; i++)
		{
			unique_ptr<Runway> temp(new Runway(i));
			runwayQueue_.push(move(temp));
		}
		for (int i = 0; i < parkingNum; i++)
		{
			unique_ptr<ParkingStand> temp(new ParkingStand(i));
			parkingQueue_.push(move(temp));
		}
	}
	//RequestLanding goes here. 
	//check if the status is true or false (hold), then if true (proceed), then assign aircraftID and start timer. 
	shared_ptr<LandingStatus> RequestLanding(int aircraftID)
	{
		lock_guard<mutex> lockResource_(muteResource_);
		if (aircraftDetails_.find(aircraftID) != aircraftDetails_.end())
		{
			return (shared_ptr<LandingStatus> (new LandingStatus(true, true)));
		}
		if (runwayQueue_.empty() || parkingQueue_.empty())
		{
			return (shared_ptr<LandingStatus> (new LandingStatus(true)));
		}

		runwayQueue_.front()->SetState(runwayState_::Reserved);
		unique_ptr<Runway> useRunway = move(runwayQueue_.front());
		parkingQueue_.front()->SetState(parkingStandState_::Reserved);
		unique_ptr<ParkingStand> useParkingStand = move(parkingQueue_.front());
		runwayQueue_.pop();
		parkingQueue_.pop();
		aircraftDetails_[aircraftID]++;

		shared_ptr<LandingStatus> tempStatus(new LandingStatus(false, aircraftID, move(useRunway), move(useParkingStand), 5));

		thread landingRequestManager([&](shared_ptr<LandingStatus> status) {
			unique_lock<mutex> threadMuter(muteResource_, defer_lock);
			this_thread::sleep_for(seconds{ status->GetExpirationTime() });
			threadMuter.lock();
			if (!status->GetUsed())
			{
				unique_ptr<Runway> runway = status->RunwayDetails();
				unique_ptr<ParkingStand> parkingStand = status->ParkingStandDetails();
				runway->SetState(runwayState_::Available);
				parkingStand->SetState(parkingStandState_::Available);
				runwayQueue_.push(move(runway));
				parkingQueue_.push(move(parkingStand));
			}
			threadMuter.unlock();
		},tempStatus);
		landingRequestManager.detach();
		return tempStatus;
	}

	//PerformLanding goes here.
	//The tokens have been received and the aircraft lands here. 
	bool PerformLanding(shared_ptr<LandingStatus> status)
	{
		unique_lock<mutex> locker(muteResource_);
		if (!status->CheckValidity())
		{
			int airID = status->GetAircraftID();
			aircraftDetails_.erase(airID);
			return false;

		}
		unique_ptr<Runway> useRunway = status->RunwayDetails();
		useRunway->SetState(runwayState_::InOperation);
		status->SetUsed();
		locker.unlock();

		thread performLandingManager([&](int aircraftID, unique_ptr<Runway> runways, unique_ptr<ParkingStand> parkingStands) {
			unique_lock<mutex> threadMuter(muteResource_, defer_lock);
			this_thread::sleep_for(seconds{ kOperationDurationSec });
			Log(aircraftID, " has landed in runway: ", runways->GetRunwayID(), " and is parking at slot: ", parkingStands->GetParkingID(),"\n");
			threadMuter.lock();
			parkingStands->SetState(parkingStandState_::Occupied);
			runways->SetState(runwayState_::Available);
			parkingStandDetails_[aircraftID] = move(parkingStands);
			runwayQueue_.push(move(runways));
			threadMuter.unlock();
		}, status->GetAircraftID(), move(useRunway), status->ParkingStandDetails());
		performLandingManager.detach();
		
		return true;
	}

	//RequestTakeoff goes here.
	//check if the status is hold or proceed and then take care of the remaining.
	shared_ptr<TakeoffStatus> RequestTakeoff(int& aircraftID)
	{
		lock_guard<mutex> locker(muteResource_);
		if (runwayQueue_.empty())
		{
			return (shared_ptr<TakeoffStatus> (new TakeoffStatus(true)));
		}

		runwayQueue_.front()->SetState(runwayState_::Reserved);
		unique_ptr<Runway> useRunway = move(runwayQueue_.front());
		runwayQueue_.pop();

		shared_ptr<TakeoffStatus> tempStatus(new TakeoffStatus(false, aircraftID, move(useRunway), 5));

		thread requestTakeoffManager([&](shared_ptr<TakeoffStatus> status)
		{
			unique_lock<mutex> threadMuter(muteResource_, defer_lock);
			this_thread::sleep_for(seconds{ status->GetExpirationTime() });
			threadMuter.lock();
			if (!status->GetUsed())
			{
				unique_ptr<Runway> runway = status->RunwayDetails();
				runway->SetState(runwayState_::Available);
				runwayQueue_.push(move(runway));
			}
			threadMuter.unlock();
		}, tempStatus);
		requestTakeoffManager.detach();
		return tempStatus;
	}

	//PerformTakeoff goes here.
	//The tokens from RequestTakeoff have been received and the runway is free for the airplane to takeoff
	bool PerformTakeoff(shared_ptr<TakeoffStatus> status)
	{
		unique_lock<mutex> locker(muteResource_);
		if (!status->CheckValidity())
		{
			return false;
		}
		unique_ptr<Runway> useRunway = status->RunwayDetails();
		unique_ptr<ParkingStand> useParkingStand = move(parkingStandDetails_[status->GetAircraftID()]);
		useRunway->SetState(runwayState_::InOperation);
		useParkingStand->SetState(parkingStandState_::Available);
		parkingQueue_.push(move(useParkingStand));
		status->SetUsed();
		locker.unlock();

		thread performTakeoffManager([&](int aircraftID, unique_ptr<Runway> runway)
		{
			unique_lock<mutex> threadMuter(muteResource_, defer_lock);
			this_thread::sleep_for(seconds{ status->GetExpirationTime() });
			Log(aircraftID, " is taking off from runway: ", runway->GetRunwayID(),"\n");
			threadMuter.lock();
			runway->SetState(runwayState_::Available);
			runwayQueue_.push(move(runway));
			aircraftDetails_.erase(aircraftID);
			threadMuter.unlock();
			Log(aircraftID, " has taken off\n");
		}, status->GetAircraftID(), move(useRunway));
		performTakeoffManager.detach();
		return true;
	}
};


int main() {
	Airport airport{15,10
		/* TODO: Initialize with runways and parking stands here. */
	};

	// Now spin a number of threads simulating some aircrafts.

	vector<shared_ptr<thread>> aircrafts;

	for (int i = 0; i < 50; i++)
	{
		aircrafts.push_back(make_shared<thread>([&airport, i]()
		{
			int id = i%35;
			for (;;)
			{

				/* TODO: Attempt calling into Airport::RequestLanding until landing token is successfully received. */
				shared_ptr<LandingStatus> landStatus = airport.RequestLanding(id);
				if (!landStatus->GetStatus())
					Log(id, " received a landing token.\n");
				else if (landStatus->Duplicate())
					Log(id, " duplicate.\n");
				else
					continue;

				// Wait some random time. In some cases the token will expire, that's OK.
				this_thread::sleep_for(seconds{ RandomInt(3) });

				/* TODO: Attempt calling into Airport::PerformLanding and break out of the loop if it's successful. Repeat otherwise. */
				bool performLandStatus = airport.PerformLanding(move(landStatus));
				if (performLandStatus)
				{
					Log("Landing Successful \n");
					break;
				}

				Log(id, " landing token expired. It will try again.\n");
			}

			// Sleep for at least the time of landing operation plus some random time
			this_thread::sleep_for(seconds{ seconds{ kOperationDurationSec + RandomInt() } });
			

			for (;;) {
				/* TODO: Attempt calling into Airport::RequestTakeoff until take-off token is successfully received. */
				//do the call here with if-else
				shared_ptr<TakeoffStatus> takeoffStatus = airport.RequestTakeoff(id);

				if (!takeoffStatus->GetStatus())
				{
					Log(id, " received a take-off token.\n");
				}
				else
				{
					//Log("Continuing \n");
					continue;
				}


				// Wait some random time. In some cases the token will expire, that's OK.
				this_thread::sleep_for(seconds{ RandomInt(3) });
			
				/* TODO: Attempt calling into Airport::PerformTakeoff and break out of the loop if it's successful. Repeat otherwise. */
				//do the call here with if-else
				bool performTakeoffStatus = airport.PerformTakeoff(move(takeoffStatus));
				if (performTakeoffStatus)
				{
					Log(id, " is taking off\n");
					break;
				}

				Log(id, " take-off token expired. It will try again.\n");
			}

		}));
	}

	for (auto aircraft : aircrafts) {
		aircraft->join();
	}
	
	Log("End of program\n");
	return 0;
}