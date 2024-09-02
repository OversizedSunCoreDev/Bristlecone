#pragma once

#include "CoreMinimal.h"
#include "FBristleconePackingSystemShim.h"
#include "FCablePackedInput.h"
#include "BristleconeCommonTypes.h"
#include <chrono> 
THIRD_PARTY_INCLUDES_START
#include "Microsoft/AllowMicrosoftPlatformTypes.h"
#include "Microsoft/HideMicrosoftPlatformTypes.h"

#include <GameInput.h>
THIRD_PARTY_INCLUDES_END
#include "Containers/CircularQueue.h"


//why do it this way?
//well, unfortunately, input in UE runs through the event loop.
//at high sample rates, this can blow the event loop to hell, 
//and it's always going to be dependent on the gamethread.
//at best, it's a tidge slow and tick-rate dependent.
//at worst, it goes boom. 
//Cabling is NOT a general purpose or complete solution. It is a thin layer
//provided as a jumping off point for working in this space.
class FCabling : public FRunnable {
public:
	FCabling();
	virtual ~FCabling() override;

	virtual bool Init() override;
	bool EvaluateAndAttemptSend(bool sent, int seqNumber, uint64_t& priorReading, uint64_t& currentRead,
	                       uint32_t sendHertzFactor);
	bool KeyboardStateMachine(IGameInputReading* reading, bool sent, int seqNumber, uint64_t& priorReading, uint64_t& currentRead,
	              uint32_t sendHertzFactor);
	bool GamepadStateMachine(IGameInputReading* reading, bool sent, int seqNumber, uint64_t& priorReading, uint64_t& currentRead, const uint32_t
	             sendHertzFactor);
	virtual uint32 Run() override;
	virtual void Exit() override;
	virtual void Stop() override;


	bool running;//cabling will let anyone unplug it. cabling is inanimate. cabling has no opinions on this.
	TSharedPtr<TCircularQueue<uint64_t>> GameThreadControlQueue;
	TSharedPtr<TCircularQueue<uint64_t>> CabledThreadControlQueue;
	FSharedEventRef WakeTransmitThread;
private:
	void Cleanup();

	};
