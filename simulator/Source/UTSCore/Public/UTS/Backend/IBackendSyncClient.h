// IBackendSyncClient — auth/config sync, heartbeat, idempotent attempt/
// telemetry upload, retry.
//
// See docs/INTERFACE_CONTRACTS.md §9. Every method here is non-blocking with
// respect to the active attempt: Local Cache and Telemetry keep writing
// regardless of this interface's state (FR-TEL-006, NFR-006). The simulator
// never writes configuration to the backend — SyncConfigurations is pull-only.

#pragma once

#include <string>
#include <vector>
#include "UTS/Common/UTSCommonTypes.h"
#include "UTS/Config/IConfigurationProvider.h"

namespace UTS
{
	enum class EBackendError : uint8_t
	{
		None,
		NetworkUnavailable,
		AuthenticationFailed,
		ConfigurationMismatch, // backend rejects a config version the station doesn't recognize
		ServerError
	};

	enum class ESyncState : uint8_t
	{
		Idle,
		Queued,
		Uploading,
		Retrying,
		Failed,
		Complete
	};

	struct FStationCredentials
	{
		FStationId StationId;
		std::string SecretOrTokenRef; // never the raw secret in logs/telemetry (NFR-005)
	};

	struct FAuthResult
	{
		bool bSucceeded = false;
		EBackendError Error = EBackendError::None;
	};

	struct FStationStatus
	{
		FStationId StationId;
		std::string SoftwareVersion;
		FSessionId ActiveSessionId;
	};

	struct FUploadReceipt
	{
		bool bSucceeded = false;
		std::string ServerRecordRef;
		EBackendError Error = EBackendError::None;
	};

	class IBackendSyncClient
	{
	public:
		virtual ~IBackendSyncClient() = default;

		virtual FAuthResult Authenticate(const FStationCredentials& Credentials) = 0;

		virtual std::vector<FConfigSnapshotEnvelope> SyncConfigurations() = 0;

		virtual void Heartbeat(const FStationStatus& Status) = 0;

		// Idempotent: a retried call with the same key never creates a
		// duplicate record server-side (FR-TEL-005, FR-API-005).
		virtual FUploadReceipt UploadAttempt(
			const FAttemptId& AttemptId, const std::string& IdempotencyKey) = 0;

		virtual FUploadReceipt UploadTelemetryChunk(
			const FAttemptId& AttemptId,
			uint32_t ChunkSeq,
			const std::string& IdempotencyKey,
			const std::vector<uint8_t>& ChunkBytes) = 0;

		virtual ESyncState GetSyncState(const FAttemptId& AttemptId) const = 0;
	};
}
