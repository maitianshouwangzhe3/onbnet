#pragma once

class no_copy_able {
public:
  no_copy_able(const no_copy_able&) = delete;
  void operator=(const no_copy_able&) = delete;

protected:
  no_copy_able() = default;
  ~no_copy_able() = default;
};
