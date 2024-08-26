// Fill out your copyright notice in the Description page of Project Settings.


#include "UBristleconeWorldSubsystem.h"

#include "Common/UdpSocketBuilder.h"



void UBristleconeWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
	Super::Initialize(Collection);
	UE_LOG(LogTemp, Warning, TEXT("Bristlecone:Subsystem: Inbound and Outbound Queues set to null."));
	QueueOfReceived = nullptr;
	QueueToSend = nullptr;
	ReceiveTimes = nullptr;
	SelfBind = nullptr;
	DebugSend = nullptr;
	//TODO @maslabgamer: does this leak memory?
	const UBristleconeConstants* ConfigVals = GetDefault<UBristleconeConstants>();
	LogOnReceive = ConfigVals->log_receive_c;
	FString address = ConfigVals->default_address_c.IsEmpty() ? "34.207.0.66" : ConfigVals->default_address_c;
	sender_runner.AddTargetAddress(address);
	UE_LOG(LogTemp, Warning, TEXT("BCN will not start unless another subsystem creates and binds queues during PostInitialize."));
	UE_LOG(LogTemp, Warning, TEXT("Bristlecone:Subsystem: Subsystem world initialized"));
}

void UBristleconeWorldSubsystem::OnWorldBeginPlay(UWorld& InWorld) {
	if ([[maybe_unused]] const UWorld* World = InWorld.GetWorld() ){
		UE_LOG(LogTemp, Warning, TEXT("Bristlecone:Subsystem: World beginning play"));
		if (!QueueToSend.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Bristlecone:Subsystem: Inbound queue not allocated. Debug mode only. Connect a controller next time?"));
			QueueToSend = MakeShareable(new TheCone::IncQ(256));
			DebugSend = QueueToSend;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Bristlecone:Subsystem: Good bind for send queue."));
		}
		
		if (!QueueOfReceived.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("Bristlecone:Subsystem: No bind for received queue. Self-binding for debug."));
			QueueOfReceived = MakeShareable(new TheCone::PacketQ(256));
			SelfBind = QueueOfReceived;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Bristlecone:Subsystem: Good bind for received queue."));
		}

		local_endpoint = FIPv4Endpoint(FIPv4Address::Any, DEFAULT_PORT);
		FUdpSocketBuilder socket_factory = FUdpSocketBuilder(TEXT("Bristlecone.Receiver.Socket"))
						.AsNonBlocking()
						.AsReusable()
						.BoundToEndpoint(local_endpoint)
						.WithReceiveBufferSize(CONTROLLER_STATE_PACKET_SIZE * 25)
						.WithSendBufferSize(CONTROLLER_STATE_PACKET_SIZE * 25);
		socketHigh = MakeShareable(socket_factory.Build());
		socketLow = MakeShareable(socket_factory.Build());
		socketBackground = MakeShareable(socket_factory.Build());

		sender_runner.SetWakeSender(WakeSender);
		// start sender thread
		//TODO: refactor this to allow proper data driven construction.
		sender_runner.BindSource(QueueToSend);
		sender_runner.SetLocalSockets(socketHigh, socketLow, socketBackground);
		sender_runner.ActivateDSCP();
		sender_thread.Reset(FRunnableThread::Create(&sender_runner, TEXT("Bristlecone.Sender")));

		// Start receiver thread
		ReceiveTimes = MakeShareable(new TheCone::TimestampQ(140));
		receiver_runner.BindStatsSink(ReceiveTimes);
		receiver_runner.LogOnReceive = LogOnReceive;
		receiver_runner.SetLocalSocket(socketHigh);
		receiver_runner.BindSink(QueueOfReceived);

		receiver_thread.Reset(FRunnableThread::Create(&receiver_runner, TEXT("Bristlecone.Receiver")));
	}
}

void UBristleconeWorldSubsystem::Deinitialize() {
	UE_LOG(LogTemp, Warning, TEXT("Bristlecone:Subsystem: Deinitializing Bristlecone subsystem"));
	
	if (sender_thread) {
		sender_runner.Stop();
		WakeSender->Trigger();
	}
	if (receiver_thread) {
		receiver_thread->Kill();
	}

	if (socketHigh.IsValid()) {
		socketHigh.Get()->Close();
	}
	if (socketLow.IsValid()) {
		socketLow.Get()->Close();
	}
	if (socketBackground.IsValid()) {
		socketBackground.Get()->Close();
	}
	socketHigh = nullptr;
	socketLow = nullptr;
	socketBackground = nullptr;

	//this will all need to be refactored, but tbh, I'm not sure we'll keep this for long enough to do it.
	FSocket* sender_socket_obj = socketHigh.Get();
	if (sender_socket_obj != nullptr) {
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(sender_socket_obj);
	}
	sender_socket_obj = socketLow.Get();
	if (sender_socket_obj != nullptr) {
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(sender_socket_obj);
	}
	sender_socket_obj = socketBackground.Get();
	if (sender_socket_obj != nullptr) {
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(sender_socket_obj);
	}
	Super::Deinitialize();
}

void UBristleconeWorldSubsystem::Tick(float DeltaTime) {
	Super::Tick(DeltaTime);
	++logTicker;
	if (logTicker >= 30)
	{
		logTicker = 0;
		if (LogOnReceive)
		{
			double sum = 0;
			double sent = 0;
			while (ReceiveTimes != nullptr && ReceiveTimes->IsEmpty() != true)
			{
				++sent;
				sum += ReceiveTimes->Peek()->first;
				ReceiveTimes->Dequeue();
			}
			UE_LOG(LogTemp, Warning, TEXT("Bristlecone: Average Latency, %lf for %lf packets"), (sum / sent), sent);
		}
	}
	//UE_LOG(LogTemp, Warning, TEXT("Bristlecone:Subsystem: Subsystem world ticked"));
}

TStatId UBristleconeWorldSubsystem::GetStatId() const {
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFBristleconeWorldSubsystem, STATGROUP_Tickables);
}
