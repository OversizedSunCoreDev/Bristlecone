﻿#pragma once

template<
	typename CLONE_TYPE,
	unsigned int CLONE_SIZE>
class FBristleconePacket;

/**
 * Acts as a wrapper to manipulate the contents of a packet for networking
 * 
 * @tparam CLONE_TYPE 
 * @tparam CLONE_SIZE 
 */
template<
	typename CLONE_TYPE,
	unsigned int CLONE_SIZE>
class FBristleconePacketContainer {
public:
	FBristleconePacketContainer(): clone_state_ring_index(0) {
	}

	void InsertNewDatagram(const CLONE_TYPE* new_datagram) {
		// Update array with new data
		CLONE_TYPE* newest_element = packet.GetPointerToElement(clone_state_ring_index);
		memcpy(newest_element, new_datagram, sizeof(CLONE_TYPE));
		packet.UpdateTransferTime();

		// Update index
		clone_state_ring_index = (clone_state_ring_index + 1) % CLONE_SIZE;
	}

	FBristleconePacket<CLONE_TYPE, CLONE_SIZE>* GetPacket() {
		return &packet;
	}

	FDateTime GetTransferTime() const {
		return packet.GetTransferTime();
	}

private:
	uint32 clone_state_ring_index;
	FBristleconePacket<CLONE_TYPE, CLONE_SIZE> packet;
};

/**
 * A Bristlecone packet, hereafter called a clone, contains at least the most recent datagram that we want to send
 * plus a history containing CLONE_SIZE - 1 datagrams
 * 
 * @tparam CLONE_TYPE Type used in the datagram.
 * @tparam CLONE_SIZE Count representing the number of datagrams to send with a single clone
 */
template<
	typename CLONE_TYPE,
	unsigned int CLONE_SIZE>
class FBristleconePacket {
public:
	FBristleconePacket() {
		Clear();
	}

	void Clear() {
		memset(clone_array, 0, sizeof(CLONE_TYPE) * CLONE_SIZE);
	}

	FDateTime GetTransferTime() const {
		return transfer_time;
	}
	
	void UpdateTransferTime() {
		transfer_time = FDateTime::Now();
	}

	CLONE_TYPE* GetPointerToElement(uint32 element_index) {
		return &clone_array[element_index];
	}

	FString ToString() const {
		FString output;// = FString::Printf(TEXT("Transfer time = %s, array = "), *transfer_time.ToString());
		output += "Transfer time = ";
		output += transfer_time.ToString();
		output += ", array = "; 
		for (uint32 array_index = 0; array_index < CLONE_SIZE; array_index++) {
			output += clone_array[array_index].ToString();
			output += " ";
		}
		return output;
	}

private:
	// Packet headers
	FDateTime transfer_time;
	// Data clone
	CLONE_TYPE clone_array[CLONE_SIZE];
};
