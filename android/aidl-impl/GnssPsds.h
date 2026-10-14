// Copyright (c) 2026 The DiamaneOS Project
// SPDX-License-Identifier: BSD-3-Clause

#ifndef ANDROID_HARDWARE_GNSS_AIDL_GNSSPSDS_H
#define ANDROID_HARDWARE_GNSS_AIDL_GNSSPSDS_H

#include <aidl/android/hardware/gnss/BnGnssPsds.h>
#include <location_interface.h>
#include <mutex>
#include <vector>

namespace android {
namespace hardware {
namespace gnss {
namespace aidl {
namespace implementation {

using ::aidl::android::hardware::gnss::BnGnssPsds;
using ::aidl::android::hardware::gnss::IGnssPsdsCallback;
using ::aidl::android::hardware::gnss::PsdsType;
using ::ndk::ScopedAStatus;
using ::std::shared_ptr;

// The framework downloads Qualcomm XTRA data as long-term PSDS and injects it
// here. When the engine asks for new data, the HAL asks the framework through
// downloadRequestCb.
struct GnssPsds : public BnGnssPsds {
    ScopedAStatus setCallback(const shared_ptr<IGnssPsdsCallback>& callback) override;
    ScopedAStatus injectPsdsData(PsdsType psdsType,
            const std::vector<uint8_t>& psdsData) override;

private:
    const GnssInterface* getGnssInterfaceLocked();
    void requestDownload();

    std::mutex mMutex;
    shared_ptr<IGnssPsdsCallback> mCallback = nullptr;
    const GnssInterface* mGnssInterface = nullptr;
};

}  // namespace implementation
}  // namespace aidl
}  // namespace gnss
}  // namespace hardware
}  // namespace android

#endif  // ANDROID_HARDWARE_GNSS_AIDL_GNSSPSDS_H
