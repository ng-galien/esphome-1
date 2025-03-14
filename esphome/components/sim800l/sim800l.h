#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "esphome/core/automation.h"

namespace esphome {
namespace sim800l {

const uint8_t SIM800L_READ_BUFFER_LENGTH = 255;

enum State {
  STATE_IDLE = 0,
  STATE_INIT,
  STATE_CHECK_AT,
  STATE_CREG,
  STATE_CREGWAIT,
  STATE_CSQ,
  STATE_CSQ_RESPONSE,
  STATE_IDLEWAIT,
  STATE_SENDINGSMS1,
  STATE_SENDINGSMS2,
  STATE_SENDINGSMS3,
  STATE_CHECK_SMS,
  STATE_PARSE_SMS,
  STATE_PARSE_SMS_RESPONSE,
  STATE_RECEIVESMS,
  STATE_READSMS,
  STATE_RECEIVEDSMS,
  STATE_DELETEDSMS,
  STATE_DISABLE_ECHO,
  STATE_PARSE_SMS_OK
};

class Sim800LComponent : public uart::UARTDevice, public PollingComponent {
 public:
  /// Retrieve the latest sensor values. This operation takes approximately 16ms.
  void update() override;
  void loop() override;
  void add_on_sms_received_callback(std::function<void(std::string, std::string)> callback) {
    this->callback_.add(std::move(callback));
  }
  void send_sms(std::string recipient, std::string message);

 protected:
  void send_cmd_(std::string);
  void parse_cmd_(std::string);

  std::string sender_;
  char read_buffer_[SIM800L_READ_BUFFER_LENGTH];
  size_t read_pos_{0};
  uint8_t parse_index_{0};
  uint8_t watch_dog_{0};
  bool expect_ack_{false};
  sim800l::State state_{STATE_IDLE};
  bool registered_{false};
  int rssi_{0};

  std::string recipient_;
  std::string outgoing_message_;
  bool send_pending_;

  CallbackManager<void(std::string, std::string)> callback_;
};

class Sim800LReceivedMessageTrigger : public Trigger<std::string, std::string> {
 public:
  explicit Sim800LReceivedMessageTrigger(Sim800LComponent *parent) {
    parent->add_on_sms_received_callback(
        [this](std::string message, std::string sender) { this->trigger(message, sender); });
  }
};

template<typename... Ts> class Sim800LSendSmsAction : public Action<Ts...> {
 public:
  Sim800LSendSmsAction(Sim800LComponent *parent) : parent_(parent) {}
  TEMPLATABLE_VALUE(std::string, recipient)
  TEMPLATABLE_VALUE(std::string, message)

  void play(Ts... x) {
    auto recipient = this->recipient_.value(x...);
    auto message = this->message_.value(x...);
    this->parent_->send_sms(recipient, message);
  }

 protected:
  Sim800LComponent *parent_;
};

}  // namespace sim800l
}  // namespace esphome
