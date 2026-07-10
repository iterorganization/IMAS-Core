// Pulse-lifecycle detail contract tests (issue #4 / TEST_STRATEGY.md §4 step 2).
//
// The scaffold and matrix suites only ever drive FORCE_CREATE_PULSE + CLOSE_PULSE
// (TRACEABILITY.md marks the other modes / al_context_info / al_get_backendID as
// gaps). This file closes those: al_begin_dataentry_action's four modes,
// al_context_info's format and ownership, al_get_backendID's happy path (and a
// real null-deref defect on the wrong context type), and al_close_pulse's
// CLOSE_PULSE vs ERASE_PULSE distinction.
//
// Per D2, the FUNCTIONALITY_INVENTORY / issue citation for every DISABLED_
// expected-fail below lives in TRACEABILITY.md (xfail bookkeeping table).
//
// Flexbuffers is excluded from the mode-matrix and erase tests: it is a pure
// in-process serialize/deserialize buffer with no persistent pulse state, so it
// does not distinguish among the four open modes at all (src/flexbuffers_backend.cpp
// openPulse only branches on read-vs-write) — the questions this file asks
// ("does OPEN_PULSE fail when absent", "does erase remove persisted state")
// don't apply to it.

#include "al_contract.h"

#include <al_lowlevel.h>
#include <al_const.h>

#include <gtest/gtest.h>

#include <cstdlib>
#include <cstring>
#include <string>

using al_contract::BackendCase;
using al_contract::PulseId;

namespace {

constexpr const char* kIds = "magnetics";

const BackendCase kLifecycleBackends[] = {
    {HDF5_BACKEND, "HDF5", /*on_disk=*/true},
    {MEMORY_BACKEND, "Memory", /*on_disk=*/false},
    {ASCII_BACKEND, "ASCII", /*on_disk=*/true},
};

// ===========================================================================
// al_begin_dataentry_action mode matrix.
// ===========================================================================
struct ModeBackendCase {
    int         id;
    const char* name;
    bool        on_disk;
    // Whether CREATE_PULSE genuinely refuses to overwrite an existing pulse
    // (true for HDF5/Memory, verified by reading HDF5Utils::createPulse and
    // MemoryBackend::openPulse; false for ASCII, a real defect — see
    // ModeKnownDefects below).
    bool        create_guards_existing;
};

inline void PrintTo(const ModeBackendCase& b, std::ostream* os) { *os << b.name; }

const ModeBackendCase kModeBackends[] = {
    {HDF5_BACKEND, "HDF5", /*on_disk=*/true, /*create_guards_existing=*/true},
    {MEMORY_BACKEND, "Memory", /*on_disk=*/false, /*create_guards_existing=*/true},
    {ASCII_BACKEND, "ASCII", /*on_disk=*/true, /*create_guards_existing=*/false},
};

class DataEntryModes : public ::testing::TestWithParam<ModeBackendCase> {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};

    // precreate_tree=false leaves nothing on disk at all — the genuinely
    // "absent" case. ASCII's OPEN_PULSE only checks that the *directory*
    // exists (ascii_backend.cpp:123-127), never that a pulse was actually
    // created there, so pre-creating the legacy tree (as every other test in
    // this fixture does, matching al_contract::TempBase's own documented
    // purpose "for FORCE_CREATE_PULSE") would make OPEN_PULSE spuriously
    // succeed on ASCII against an empty directory.
    std::string uri(bool precreate_tree = true) const {
        const ModeBackendCase& b = GetParam();
        if (precreate_tree && b.on_disk) base_.make_legacy_tree(pulse_);
        return al_contract::build_uri(b.id, base_.str(), pulse_);
    }
};

TEST_P(DataEntryModes, OpenPulseFailsWhenAbsent) {
    const std::string u = uri(/*precreate_tree=*/false);
    ASSERT_FALSE(u.empty());
    int pctx = -1;
    al_status_t s = al_begin_dataentry_action(u.c_str(), OPEN_PULSE, &pctx);
    EXPECT_NE(s.code, 0) << "OPEN_PULSE must fail when no pulse exists yet";
    if (s.code == 0) al_close_pulse(pctx, CLOSE_PULSE);
}

TEST_P(DataEntryModes, ForceOpenPulseCreatesWhenAbsent) {
    const std::string u = uri();
    ASSERT_FALSE(u.empty());
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(u.c_str(), FORCE_OPEN_PULSE, &pctx));
    EXPECT_EQ(al_close_pulse(pctx, CLOSE_PULSE).code, 0);
}

TEST_P(DataEntryModes, OpenPulseSucceedsOncePresent) {
    const std::string u = uri();
    ASSERT_FALSE(u.empty());
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx));
    EXPECT_EQ(al_close_pulse(pctx, CLOSE_PULSE).code, 0);

    int pctx2 = -1;
    AL_EXPECT_OK(al_begin_dataentry_action(u.c_str(), OPEN_PULSE, &pctx2));
    EXPECT_EQ(al_close_pulse(pctx2, CLOSE_PULSE).code, 0);
}

TEST_P(DataEntryModes, CreatePulseFailsWhenAlreadyExists) {
    const ModeBackendCase& b = GetParam();
    if (!b.create_guards_existing) {
        GTEST_SKIP() << "known defect for " << b.name
                     << " — see ModeKnownDefects.*";
    }
    const std::string u = uri();
    ASSERT_FALSE(u.empty());
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx));
    EXPECT_EQ(al_close_pulse(pctx, CLOSE_PULSE).code, 0);

    int pctx2 = -1;
    al_status_t s = al_begin_dataentry_action(u.c_str(), CREATE_PULSE, &pctx2);
    EXPECT_NE(s.code, 0) << "CREATE_PULSE must refuse to overwrite an existing pulse";
    if (s.code == 0) al_close_pulse(pctx2, CLOSE_PULSE);
}

INSTANTIATE_TEST_SUITE_P(
    Backends, DataEntryModes, ::testing::ValuesIn(kModeBackends),
    [](const ::testing::TestParamInfo<ModeBackendCase>& info) {
        return std::string(info.param.name);
    });

// --- ASCII: CREATE_PULSE silently truncates instead of refusing ------------
TEST(ModeKnownDefects, DISABLED_AsciiCreatePulseFailsWhenAlreadyExists) {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string u = al_contract::build_uri(ASCII_BACKEND, base.str(), pulse);
    int pctx = -1;
    ASSERT_EQ(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
    ASSERT_EQ(al_close_pulse(pctx, CLOSE_PULSE).code, 0);

    int pctx2 = -1;
    al_status_t s = al_begin_dataentry_action(u.c_str(), CREATE_PULSE, &pctx2);
    EXPECT_NE(s.code, 0)
        << "CREATE_PULSE must refuse an already-existing pulse (documented: "
           "\"create a new pulse, do not overwrite if already exist\")";
    if (s.code == 0) al_close_pulse(pctx2, CLOSE_PULSE);
}

TEST(ModeKnownDefects, AsciiCreatePulseCurrentlyOverwritesSilently) {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string u = al_contract::build_uri(ASCII_BACKEND, base.str(), pulse);
    int pctx = -1;
    ASSERT_EQ(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
    int op = -1;
    al_begin_global_action(pctx, kIds, "", WRITE_OP, &op);
    al_contract::write_data<double>(op, "leaf", {}, {11.0});
    al_end_action(op);
    ASSERT_EQ(al_close_pulse(pctx, CLOSE_PULSE).code, 0);

    int pctx2 = -1;
    al_status_t s = al_begin_dataentry_action(u.c_str(), CREATE_PULSE, &pctx2);
    EXPECT_EQ(s.code, 0)
        << "AsciiBackend::openPulse now guards CREATE_PULSE against an "
           "existing pulse — enable "
           "ModeKnownDefects.DISABLED_AsciiCreatePulseFailsWhenAlreadyExists "
           "(ascii_backend.cpp:107-157 has no existence check for CREATE_PULSE)";
    if (s.code == 0) al_close_pulse(pctx2, CLOSE_PULSE);
}

// ===========================================================================
// al_context_info: format per context type, NULL-context special case,
// malloc ownership (src/al_lowlevel.cpp:828-832 -> free() is correct).
// ===========================================================================
class ContextInfo : public ::testing::Test {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};
};

TEST_F(ContextInfo, NullContextReturnsLiteralString) {
    char* info = nullptr;
    AL_ASSERT_OK(al_context_info(0, &info));
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info, "NULL context");
    free(info);
}

TEST_F(ContextInfo, PulseContextDescribesItsUri) {
    base_.make_legacy_tree(pulse_);
    const std::string u = al_contract::build_uri(HDF5_BACKEND, base_.str(), pulse_);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx));

    char* info = nullptr;
    AL_ASSERT_OK(al_context_info(pctx, &info));
    ASSERT_NE(info, nullptr);
    EXPECT_NE(std::string(info).find("uri"), std::string::npos)
        << "pulse context description must mention its uri: " << info;
    free(info);

    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST_F(ContextInfo, OperationContextDescribesDataobjectAndAccessmode) {
    base_.make_legacy_tree(pulse_);
    const std::string u = al_contract::build_uri(HDF5_BACKEND, base_.str(), pulse_);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx));
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));

    char* info = nullptr;
    AL_ASSERT_OK(al_context_info(op, &info));
    ASSERT_NE(info, nullptr);
    const std::string s(info);
    EXPECT_NE(s.find("dataobjectname"), std::string::npos) << s;
    EXPECT_NE(s.find("accessmode"), std::string::npos) << s;
    EXPECT_NE(s.find(kIds), std::string::npos) << s;
    free(info);

    al_end_action(op);
    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST_F(ContextInfo, ArraystructContextDescribesPathAndIndex) {
    base_.make_legacy_tree(pulse_);
    const std::string u = al_contract::build_uri(HDF5_BACKEND, base_.str(), pulse_);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx));
    int op = -1;
    AL_ASSERT_OK(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op));
    int size = 1;
    int aos = -1;
    AL_ASSERT_OK(al_begin_arraystruct_action(op, "elements", "", &size, &aos));

    char* info = nullptr;
    AL_ASSERT_OK(al_context_info(aos, &info));
    ASSERT_NE(info, nullptr);
    const std::string s(info);
    EXPECT_NE(s.find("path"), std::string::npos) << s;
    EXPECT_NE(s.find("index"), std::string::npos) << s;
    free(info);

    al_end_action(aos);
    al_end_action(op);
    al_close_pulse(pctx, CLOSE_PULSE);
}

// ===========================================================================
// al_build_uri_from_legacy_parameters: caller frees `*uri` (malloc-based,
// src/al_context.cpp:241-261 `malloc`+`memcpy`+NUL) — issue #8 ownership
// sweep. al_contract::build_uri already frees it on every call across this
// whole suite; this test pins the ownership contract explicitly by calling
// the raw ABI function directly instead of through that helper, mirroring
// ContextInfo's explicit-free style above.
// ===========================================================================
TEST(UriOwnership, CallerFreesBuiltUri) {
    al_contract::TempBase base;
    PulseId pulse{/*database=*/"test", /*version=*/"3", /*pulse=*/12,
                  /*run=*/0};
    char* uri = nullptr;
    al_status_t s = al_build_uri_from_legacy_parameters(
        HDF5_BACKEND, pulse.pulse, pulse.run, base.str().c_str(),
        pulse.database.c_str(), pulse.version.c_str(), "", &uri);
    AL_ASSERT_OK(s);
    ASSERT_NE(uri, nullptr);
    EXPECT_NE(std::string(uri).find("pulse=12"), std::string::npos) << uri;
    free(uri);
}

// ===========================================================================
// al_get_backendID.
// ===========================================================================
TEST(GetBackendId, ReturnsTheBackendUsedToOpen) {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string u = al_contract::build_uri(HDF5_BACKEND, base.str(), pulse);
    int pctx = -1;
    AL_ASSERT_OK(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx));

    int beid = -1;
    AL_ASSERT_OK(al_get_backendID(pctx, &beid));
    EXPECT_EQ(beid, HDF5_BACKEND);

    al_close_pulse(pctx, CLOSE_PULSE);
}

// --- known defect: al_get_backendID silently accepts a non-pulse context ---
// al_lowlevel.cpp's al_get_backendID reinterprets ctxID's context via
// `static_cast<DataEntryContext*>(lle.context)` — NOT a dynamic_cast, and with
// no type check at all (unlike al_close_pulse / al_begin_dataentry_action /
// al_delete_data, which all dynamic_cast and explicitly guard against a null
// result). Confirmed empirically (a debug build against this exact call):
// passing an OperationContext does NOT crash — the cast is well-formed
// pointer arithmetic, so it silently reads whatever bytes sit at
// DataEntryContext::backend_id's offset inside the unrelated object and
// returns them as if they were valid, with status.code==0. That value is
// undefined behavior (not a stable "wrong number" to assert), so the only
// portable part of the defect to pin is that the call reports *success* at
// all for a context of the wrong type.
TEST(GetBackendIdKnownDefects, DISABLED_WrongContextTypeReturnsError) {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string u = al_contract::build_uri(HDF5_BACKEND, base.str(), pulse);
    int pctx = -1;
    ASSERT_EQ(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
    int op = -1;
    ASSERT_EQ(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op).code, 0);

    int beid = -1;
    al_status_t s = al_get_backendID(op, &beid);
    EXPECT_NE(s.code, 0)
        << "al_get_backendID on a non-pulse context must return an error";

    al_end_action(op);
    al_close_pulse(pctx, CLOSE_PULSE);
}

TEST(GetBackendIdKnownDefects, WrongContextTypeCurrentlySucceedsViaUnsafeCast) {
    al_contract::TempBase base;
    PulseId pulse{"test", "3", 12, 0};
    base.make_legacy_tree(pulse);
    const std::string u = al_contract::build_uri(HDF5_BACKEND, base.str(), pulse);
    int pctx = -1;
    ASSERT_EQ(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
    int op = -1;
    ASSERT_EQ(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op).code, 0);

    int beid = -1;
    al_status_t s = al_get_backendID(op, &beid);
    EXPECT_EQ(s.code, 0)
        << "al_get_backendID now validates context type — enable "
           "GetBackendIdKnownDefects.DISABLED_WrongContextTypeReturnsError "
           "(al_lowlevel.cpp al_get_backendID uses an unchecked static_cast)";

    al_end_action(op);
    al_close_pulse(pctx, CLOSE_PULSE);
}

// ===========================================================================
// al_close_pulse: CLOSE_PULSE vs ERASE_PULSE.
// ===========================================================================
// None of the three always-on persistent backends actually branch on `mode`
// in closePulse (verified by reading HDF5Reader/Writer::closePulse,
// AsciiBackend::closePulse, and MemoryBackend::closePulse, none of which
// reference their mode parameter) — ERASE_PULSE behaves exactly like
// CLOSE_PULSE. The oracle here is backend-agnostic and avoids hard-coding any
// on-disk path convention: after ERASE_PULSE, a plain (non-FORCE) OPEN_PULSE
// on the same URI is the documented signal that the pulse is gone.
bool erase_then_plain_open_succeeds(const BackendCase& b,
                                    al_contract::TempBase& base,
                                    const PulseId& pulse) {
    if (b.on_disk) base.make_legacy_tree(pulse);
    const std::string u = al_contract::build_uri(b.id, base.str(), pulse);
    int pctx = -1;
    EXPECT_EQ(al_begin_dataentry_action(u.c_str(), FORCE_CREATE_PULSE, &pctx).code, 0);
    int op = -1;
    EXPECT_EQ(al_begin_global_action(pctx, kIds, "", WRITE_OP, &op).code, 0);
    EXPECT_EQ(al_contract::write_data<double>(op, "leaf", {}, {11.0}).code, 0);
    EXPECT_EQ(al_end_action(op).code, 0);
    EXPECT_EQ(al_close_pulse(pctx, ERASE_PULSE).code, 0);

    int pctx2 = -1;
    al_status_t s = al_begin_dataentry_action(u.c_str(), OPEN_PULSE, &pctx2);
    if (s.code == 0) al_close_pulse(pctx2, CLOSE_PULSE);
    return s.code == 0;
}

class ErasePulseKnownDefects : public ::testing::Test {
protected:
    al_contract::TempBase base_;
    PulseId pulse_{/*database=*/"test", /*version=*/"3", /*pulse=*/12, /*run=*/0};
};

TEST_F(ErasePulseKnownDefects, DISABLED_Hdf5EraseMakesPulseUnopenable) {
    EXPECT_FALSE(erase_then_plain_open_succeeds(kLifecycleBackends[0], base_, pulse_))
        << "ERASE_PULSE must remove the pulse so a later plain OPEN_PULSE fails";
}
TEST_F(ErasePulseKnownDefects, Hdf5EraseCurrentlyLeavesPulseOpenable) {
    EXPECT_TRUE(erase_then_plain_open_succeeds(kLifecycleBackends[0], base_, pulse_))
        << "HDF5Reader/Writer::closePulse now honors ERASE_PULSE — enable "
           "DISABLED_Hdf5EraseMakesPulseUnopenable "
           "(closePulse never references its mode parameter today)";
}

TEST_F(ErasePulseKnownDefects, DISABLED_AsciiEraseMakesPulseUnopenable) {
    EXPECT_FALSE(erase_then_plain_open_succeeds(kLifecycleBackends[2], base_, pulse_))
        << "ERASE_PULSE must remove the pulse so a later plain OPEN_PULSE fails";
}
TEST_F(ErasePulseKnownDefects, AsciiEraseCurrentlyLeavesPulseOpenable) {
    EXPECT_TRUE(erase_then_plain_open_succeeds(kLifecycleBackends[2], base_, pulse_))
        << "AsciiBackend::closePulse now honors ERASE_PULSE — enable "
           "DISABLED_AsciiEraseMakesPulseUnopenable "
           "(closePulse ignores its mode parameter today)";
}

TEST_F(ErasePulseKnownDefects, DISABLED_MemoryEraseMakesPulseUnopenable) {
    EXPECT_FALSE(erase_then_plain_open_succeeds(kLifecycleBackends[1], base_, pulse_))
        << "ERASE_PULSE must remove the pulse so a later plain OPEN_PULSE fails";
}
TEST_F(ErasePulseKnownDefects, MemoryEraseCurrentlyLeavesPulseOpenable) {
    EXPECT_TRUE(erase_then_plain_open_succeeds(kLifecycleBackends[1], base_, pulse_))
        << "MemoryBackend::closePulse now honors ERASE_PULSE — enable "
           "DISABLED_MemoryEraseMakesPulseUnopenable "
           "(closePulse is an empty function body today)";
}

}  // namespace
