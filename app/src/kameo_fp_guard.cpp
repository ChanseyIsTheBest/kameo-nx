// kameo-nx (2026-11-02), from Kameo: RePowered's src/core/fp_guard.cpp (MaxDeadBear, BiRabittoh; MIT): on the PC a guest
// thread's MXCSR can have FZ and DAZ set and the exception mask bits clear (the first run stopped with C000008F, an inexact
// result, in generated vector code, sub_8234A168), so a guest computation faults on the host. The handler masks them in
// the faulting context and carries on. The Switch has nothing to guard: the A57 does not trap floating-point exceptions.
#if defined(_WIN32)
#include <Windows.h>
#include <xmmintrin.h>

#include <atomic>
#include <cstdint>

namespace {

std::atomic<uint64_t> g_fp_exceptions{0};

LONG WINAPI GuestFpExceptionHandler(EXCEPTION_POINTERS *ep) {
  switch (ep->ExceptionRecord->ExceptionCode) {
    case 0xC000008C:  // STATUS_FLOAT_DIVIDE_BY_ZERO
    case 0xC000008D:  // STATUS_FLOAT_OVERFLOW
    case 0xC000008E:  // STATUS_FLOAT_UNDERFLOW
    case 0xC000008F:  // STATUS_FLOAT_INEXACT_RESULT
    case 0xC0000090:  // STATUS_FLOAT_INVALID_OPERATION
    case 0xC0000091:  // STATUS_FLOAT_STACK_CHECK
      g_fp_exceptions.fetch_add(1, std::memory_order_relaxed);
      ep->ContextRecord->MxCsr |= _MM_MASK_MASK;
      return EXCEPTION_CONTINUE_EXECUTION;
    default:
      return EXCEPTION_CONTINUE_SEARCH;
  }
}

}  // namespace

void KameoInstallFpGuard() {
  AddVectoredExceptionHandler(1, GuestFpExceptionHandler);
}
uint64_t KameoFpExceptionCount() {
  return g_fp_exceptions.load(std::memory_order_relaxed);
}
#else
#include <cstdint>
void KameoInstallFpGuard() {}
uint64_t KameoFpExceptionCount() {
  return 0;
}
#endif
