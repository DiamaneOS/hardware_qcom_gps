// Copyright (c) 2026 The DiamaneOS Project
// SPDX-License-Identifier: BSD-3-Clause

#define LOG_TAG "GnssPsdsAidl"

#include "GnssPsds.h"
#include <aidl/android/hardware/gnss/IGnss.h>
#include <log_util.h>
#include "loc_misc_utils.h"

typedef const GnssInterface* (getLocationInterface)();

namespace android {
namespace hardware {
namespace gnss {
namespace aidl {
namespace implementation {

using ::aidl::android::hardware::gnss::IGnss;

// The framework's PSDS downloader accepts at most this many bytes.
static constexpr size_t kMaxPsdsDataSize = 1000000;

const GnssInterface* GnssPsds::getGnssInterfaceLocked() {
    if (nullptr == mGnssInterface) {
        void* libHandle = nullptr;
        getLocationInterface* getter = (getLocationInterface*)
                dlGetSymFromLib(libHandle, "libgnss.so", "getGnssInterface");
        if (nullptr != getter) {
            mGnssInterface = (*getter)();
        }
    }
    return mGnssInterface;
}

ScopedAStatus GnssPsds::setCallback(const shared_ptr<IGnssPsdsCallback>& callback) {
    if (nullptr == callback) {
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_ARGUMENT);
    }

    std::lock_guard<std::mutex> lock(mMutex);
    const GnssInterface* gnssInterface = getGnssInterfaceLocked();
    if (nullptr == gnssInterface) {
        LOC_LOGe("No GNSS interface");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }
    mCallback = callback;

    std::weak_ptr<GnssPsds> weakThis = ref<GnssPsds>();
    gnssInterface->setXtraDataRequestCb([weakThis]() {
        shared_ptr<GnssPsds> psds = weakThis.lock();
        if (nullptr != psds) {
            psds->requestDownload();
        }
    });
    return ScopedAStatus::ok();
}

ScopedAStatus GnssPsds::injectPsdsData(PsdsType psdsType,
        const std::vector<uint8_t>& psdsData) {
    // XTRA is the only kind of data the engine takes, and it is long-term data.
    if (PsdsType::LONG_TERM != psdsType || psdsData.empty() ||
            psdsData.size() > kMaxPsdsDataSize) {
        LOC_LOGe("PSDS data not injected: type %d, %zu bytes",
                static_cast<int>(psdsType), psdsData.size());
        return ScopedAStatus::fromServiceSpecificError(IGnss::ERROR_INVALID_ARGUMENT);
    }

    std::unique_lock<std::mutex> lock(mMutex);
    const GnssInterface* gnssInterface = getGnssInterfaceLocked();
    lock.unlock();
    if (nullptr == gnssInterface) {
        LOC_LOGe("No GNSS interface");
        return ScopedAStatus::fromExceptionCode(EX_ILLEGAL_STATE);
    }

    LOC_LOGi("Injecting %zu bytes of PSDS data", psdsData.size());
    gnssInterface->injectXtraData(psdsData);
    return ScopedAStatus::ok();
}

// Called on the GNSS adapter thread when the engine asks for XTRA data.
void GnssPsds::requestDownload() {
    std::unique_lock<std::mutex> lock(mMutex);
    shared_ptr<IGnssPsdsCallback> callback = mCallback;
    lock.unlock();

    if (nullptr != callback && !callback->downloadRequestCb(PsdsType::LONG_TERM).isOk()) {
        LOC_LOGe("downloadRequestCb failed");
    }
}

}  // namespace implementation
}  // namespace aidl
}  // namespace gnss
}  // namespace hardware
}  // namespace android
