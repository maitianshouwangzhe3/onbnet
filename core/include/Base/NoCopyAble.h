#pragma once

class NonCopyAble {
public:
  NonCopyAble(const NonCopyAble&) = delete;
  void operator=(const NonCopyAble&) = delete;

protected:
  NonCopyAble() = default;
  ~NonCopyAble() = default;
};
