#include "FBristleconeSender.h"

#include "UBristleconeWorldSubsystem.h"
#include "Common/UdpSocketBuilder.h"

//these includes shouldn't be moved to the .h, due to odd declaration behaviors.
//the same pattern can be seen, executed differently, in the socket library.
//it looks like they basically "shade" them from being included in some TLUs.
#if PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS
#include "Windows/WindowsHWrapper.h"
#include "Windows/AllowWindowsPlatformTypes.h"
#include <winsock2.h>
#include <ws2tcpip.h>
#include <qos2.h>

typedef int32 SOCKLEN;

#include "Windows/HideWindowsPlatformTypes.h"

#endif
#include <Runtime/Sockets/Private/BSDSockets/SocketsBSD.h>

FBristleconeSender::FBristleconeSender()
: consecutive_zero_bytes_sent(0), running(false) {
	UE_LOG(LogTemp, Display, TEXT("Bristlecone:Sender: Constructing Bristlecone Sender"));

	target_endpoints.Reserve(MAX_TARGET_COUNT);
}

FBristleconeSender::~FBristleconeSender() {
	UE_LOG(LogTemp, Display, TEXT("Bristlecone:Sender: Destructing Bristlecone Sender"));
}

void FBristleconeSender::BindSource(TheCone::SendQueue QueueCandidate)
{
	Queue.Reset();
	Queue = QueueCandidate;
}

void FBristleconeSender::AddTargetAddress(FString target_address_str) {
	FIPv4Address target_address;
	FIPv4Address::Parse(target_address_str, target_address);
	FIPv4Endpoint target_endpoint = FIPv4Endpoint(target_address, DEFAULT_PORT);
	target_endpoints.Emplace(target_endpoint);
}

void FBristleconeSender::SetLocalSockets(
	const TSharedPtr<FSocket, ESPMode::ThreadSafe>& new_socket_high,
	const TSharedPtr<FSocket, ESPMode::ThreadSafe>& new_socket_low,
	const TSharedPtr<FSocket, ESPMode::ThreadSafe>& new_socket_adaptive
) {
	sender_socket_high = new_socket_high;
	sender_socket_low = new_socket_low;
	sender_socket_background = new_socket_adaptive;
}

void FBristleconeSender::SetWakeSender(FSharedEventRef NewWakeSender) {
	WakeSender = NewWakeSender;
}

void FBristleconeSender::ActivateDSCP()
{

	//this no-ops on many platforms.
	//TODO: If you want to take this to general production grade, this will need a behavior for most platforms.
	//Fortunately, Linux, mac, steam deck, and many other platforms will actually be simpler, as those allow dscp
	//to be set normally, instead of requiring qos manipulation.
	// 
	//The outliers are switch and Steam Datagram Relays, and I don't even know that you'd ever use bristlecone with SDR, as it's basically
	// a successor system with a narrow application space.
	
	//quite a lot of ungood things have to happen for us to do this. As a result, I'll be writing out what we're doing.
	// https://learn.microsoft.com/en-us/windows/win32/api/qos2/nf-qos2-qosaddsockettoflow
	//https://github.com/microsoft/Windows-classic-samples/blob/main/Samples/Win7Samples/netds/Qos/Qos2/qossample.c
	//This is probably the best example I can provide for WHAT is happening.
	//DSCP settings do appear to have a significant but small effect on behavior, contrary to popular wisdom.
#if PLATFORM_HAS_BSD_SOCKET_FEATURE_WINSOCKETS
	QOS_VERSION Version;
	SOCKADDR_IN destination;
	HANDLE      QoSHandle = NULL;


	// Initialize the QoS version parameter.
	Version.MajorVersion = 1;
	Version.MinorVersion = 0;
	QOS_FLOWID     QoSFlowId = 0;
	destination.sin_family = AF_INET;
	destination.sin_port = target_endpoints.Last().Port;
	destination.sin_addr.s_addr = target_endpoints.Last().Address.Value;
	// Get a handle to the QoS subsystem. this requires us to have the qwave lib file loaded to resolve the symbol. Oddly, you can't load the dll.

	QOSCreateHandle(
		&Version,
		&QoSHandle);
	//this is necessary because fsocket does not have a get native, as not all abstracted sockets actually have a native file-like socket
	//our build mechanism has to break encapsulation pretty aggressively to resolve this, and it's quite ugly.
	SOCKET underlyingHigh = ((FSocketBSD*)(sender_socket_high.Get()))->GetNativeSocket();// Time to go for a very bad ride.
	SOCKET underlyingLow = ((FSocketBSD*)(sender_socket_low.Get()))->GetNativeSocket();
	SOCKET underlyingSPICY = ((FSocketBSD*)(sender_socket_background.Get()))->GetNativeSocket();
	//qwave MAY need destination and point. as a result, we had to wait to perform this until now.
	//https://learn.microsoft.com/en-us/windows/win32/api/qos2/ne-qos2-qos_traffic_type
	//The DSCP markings are the most effective part, so far as I can tell, but local routers often support 802.1.
	//we should switch to using https://learn.microsoft.com/en-us/windows/win32/api/qos2/nf-qos2-qossetflow
	//and revisit this. It appears that you can override bandwidth limits that may be automatically placed on flows
	// and get system RTT information for flows. Both of these would be useful. the windows QoS system is always running
	//to some extent and CAN delay traffic from being sent to meet traffic shaping goals.
	// see https://learn.microsoft.com/en-us/windows/win32/api/qos2/ne-qos2-qos_shaping
	// We'd like to modify this aggressively, but that's more testing and research than I can afford atm.
	
	//Codepoints worth testing still are: some combinations of (4, 7, 11, 18, 23)
	//which cisco docs indicate are supported AHBs. I'm not sure how to get windows to set and respect those without admin.

	//While there are other codings for gaming, our control flow is actually most similar to low bit-rate av flows
	//where jitter is crippling. We can't mark all our packets at codepoint 46, or we may get shaped and dropped for being
	//bad citizens, but we do want to mark one of our clones at 46.
	QOSAddSocketToFlow(QoSHandle, underlyingHigh, (SOCKADDR*)&destination, QOS_TRAFFIC_TYPE::QOSTrafficTypeAudioVideo, QOS_NON_ADAPTIVE_FLOW, &QoSFlowId);
	QoSFlowId = 0;
	QOSAddSocketToFlow(QoSHandle, underlyingLow, (SOCKADDR*)&destination, QOS_TRAFFIC_TYPE::QOSTrafficTypeAudioVideo, QOS_NON_ADAPTIVE_FLOW, &QoSFlowId);
	QoSFlowId = 0;
	//Control doesn't seem to actually set a respected value. Unfortunately, I can't find a way to set an arbitrary DSCP without admin on windows.
	QOSAddSocketToFlow(QoSHandle, underlyingSPICY, (SOCKADDR*)&destination, QOS_TRAFFIC_TYPE::QOSTrafficTypeBackground, QOS_NON_ADAPTIVE_FLOW, &QoSFlowId);
#endif
}

bool FBristleconeSender::Init() {
	UE_LOG(LogTemp, Display, TEXT("Bristlecone:Sender: Initializing Bristlecone Sender thread"));
	socket_subsystem.Reset(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM));
	
	running = true;
	return true;
}

uint32 FBristleconeSender::Run() {
	int counter = 0;
	FControllerState sending_state;
	
	while(sender_socket_high) {
		WakeSender->Reset();
		// Update ring array
		//BRRRRRRRRRRRRRRRRRRRRRRRRRRRRRRR
		while(!Queue.Get()->IsEmpty())
		{
			++counter;
			sending_state.controller_arr = *Queue->Peek(); //assign by value or you'll have a bad time.
			packet_container.InsertNewDatagram(&sending_state);
			packet_container.GetPacket()->UpdateCycleOrMeta(counter);
			for (auto& endpoint : target_endpoints) {
				int32 bytes_sent;
				//we may want a mode to timestamp each individual packet during testing so we can get a unique rtt
				const FControllerStatePacket* current_controller_state = packet_container.GetPacket();
				auto holdopen = endpoint;
				bool packet_sent = sender_socket_high->SendTo(reinterpret_cast<const uint8*>(current_controller_state), sizeof(FControllerStatePacket),
					bytes_sent, *holdopen.ToInternetAddr());
				packet_sent = sender_socket_low->SendTo(reinterpret_cast<const uint8*>(current_controller_state), sizeof(FControllerStatePacket),
					bytes_sent, *holdopen.ToInternetAddr());
				packet_sent = sender_socket_background->SendTo(reinterpret_cast<const uint8*>(current_controller_state), sizeof(FControllerStatePacket),
					bytes_sent, *holdopen.ToInternetAddr());


				if (bytes_sent == 0) {
					consecutive_zero_bytes_sent++;
				}
				else {
					consecutive_zero_bytes_sent = 0;
				}

			}
			Queue->Dequeue();
		}
		WakeSender->Wait(8);
	}
	
	return 0;
}

void FBristleconeSender::Exit() {
	UE_LOG(LogTemp, Display, TEXT("Bristlecone:Sender: Exiting Bristlecone sender thread."));
	running = false;
	Cleanup();
}

void FBristleconeSender::Stop() {
	UE_LOG(LogTemp, Display, TEXT("Bristlecone:Sender: Stopping Bristlecone sender thread."));
	running = false;
	Cleanup();
}

void FBristleconeSender::Cleanup() {
	sender_socket_high = nullptr;
	sender_socket_low = nullptr;
	sender_socket_background = nullptr;
	const ISocketSubsystem* socket_subsystem_obj = socket_subsystem.Release();
	if (socket_subsystem_obj != nullptr) {
		socket_subsystem_obj = nullptr;
	}
	target_endpoints.Empty();
	running = false;
}

