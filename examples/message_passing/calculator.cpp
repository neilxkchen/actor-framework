/******************************************************************************\
 * This example is a very basic, non-interactive math service implemented     *
 * for both the blocking and the event-based API.                             *
\******************************************************************************/

// This example is partially included in the manual, do not modify
// without updating the references in the *.tex files!
// Manual references: lines 17-21, 24-29, 31-71, 73-107, and 140-146 (Actor.tex)

#include <iostream>

#include "caf/all.hpp"

using std::endl;
using namespace caf;

using add_atom = atom_constant<atom("add")>;
using sub_atom = atom_constant<atom("sub")>;

using calculator_actor = typed_actor<replies_to<add_atom, int, int>::with<int>,
                                     replies_to<sub_atom, int, int>::with<int>>;

// prototypes and forward declarations
behavior calculator_fun(event_based_actor* self);
void blocking_calculator_fun(blocking_actor* self);
calculator_actor::behavior_type typed_calculator_fun();
class calculator;
class blocking_calculator;
class typed_calculator;

// function-based, dynamically typed, event-based API
behavior calculator_fun(event_based_actor* self) {
  return behavior{
    [](add_atom, int a, int b) {
      return a + b;
    },
    [](sub_atom, int a, int b) {
      return a - b;
    },
    others >> [=](const message& msg) {
      aout(self) << "received: " << to_string(msg) << endl;
    }
  };
}

// function-based, dynamically typed, blocking API
void blocking_calculator_fun(blocking_actor* self) {
  self->receive_loop (
    [](add_atom, int a, int b) {
      return a + b;
    },
    [](sub_atom, int a, int b) {
      return a - b;
    },
    others >> [=](const message& msg) {
      aout(self) << "received: " << to_string(msg) << endl;
    }
  );
}

// function-based, statically typed, event-based API
calculator_actor::behavior_type typed_calculator_fun() {
  return {
    [](add_atom, int a, int b) {
      return a + b;
    },
    [](sub_atom, int a, int b) {
      return a - b;
    }
  };
}

// class-based, dynamically typed, event-based API
class calculator : public event_based_actor {
public:
  calculator(actor_config& cfg) : event_based_actor(cfg) {
    // nop
  }

  behavior make_behavior() override {
    return calculator_fun(this);
  }
};

// class-based, dynamically typed, blocking API
class blocking_calculator : public blocking_actor {
public:
  blocking_calculator(actor_config& cfg) : blocking_actor(cfg) {
    // nop
  }

  void act() override {
    blocking_calculator_fun(this);
  }
};

// class-based, statically typed, event-based API
class typed_calculator : public calculator_actor::base {
public:
  typed_calculator(actor_config& cfg) : calculator_actor::base(cfg) {
    // nop
  }

  behavior_type make_behavior() override {
    return typed_calculator_fun();
  }
};

void tester(scoped_actor&) {
  // end of recursion
}

// tests a calculator instance
template <class Handle, class... Ts>
void tester(scoped_actor& self, const Handle& hdl, int x, int y, Ts&&... xs) {
  self->monitor(hdl);
  // first test: x + y = z
  self->request(hdl, indefinite, add_atom::value, x, y).receive(
    [&](int res1) {
      aout(self) << x << " + " << y << " = " << res1 << endl;
      // second test: x - y = z
      self->request(hdl, indefinite, sub_atom::value, x, y).receive(
        [&](int res2) {
          aout(self) << x << " - " << y << " = " << res2 << endl;
          self->send_exit(hdl, exit_reason::user_shutdown);
        }
      );
    },
    [&](const error& err) {
      aout(self) << "AUT (actor under test) failed: "
                 << self->system().render(err) << endl;
      self->quit(exit_reason::user_shutdown);
    }
  );
  self->receive([](const down_msg&) {});
  tester(self, std::forward<Ts>(xs)...);
}

int main() {
  actor_system system;
  auto a1 = system.spawn(blocking_calculator_fun);
  auto a2 = system.spawn(calculator_fun);
  auto a3 = system.spawn(typed_calculator_fun);
  auto a4 = system.spawn<blocking_calculator>();
  auto a5 = system.spawn<calculator>();
  auto a6 = system.spawn<typed_calculator>();
  scoped_actor self{system};
  tester(self, a1, 1, 2, a2, 3, 4, a3, 5, 6, a4, 7, 8, a5, 9, 10, a6, 11, 12);
}
