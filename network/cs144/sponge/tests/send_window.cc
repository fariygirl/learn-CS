#include "sender_harness.hh"
#include "wrapping_integers.hh"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

using namespace std;

int main() {
  try {
    auto rd = get_random_generator();

    {
      TCPConfig cfg;
      WrappingInt32 isn(rd());
      cfg.fixed_isn = isn;

      TCPSenderTestHarness test{
          "Initial receiver advertised window is respected", cfg};
      test.execute(ExpectSegment{}
                       .with_no_flags()
                       .with_syn(true)
                       .with_payload_size(0)
                       .with_seqno(isn));
      test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(4));
      test.execute(ExpectNoSegment{});
      test.execute(WriteBytes{"abcdefg"});
      test.execute(ExpectSegment{}.with_no_flags().with_data("abcd"));
      test.execute(ExpectNoSegment{});
    }

    {
      TCPConfig cfg;
      WrappingInt32 isn(rd());
      cfg.fixed_isn = isn;

      TCPSenderTestHarness test{"Immediate window is respected", cfg};
      test.execute(ExpectSegment{}
                       .with_no_flags()
                       .with_syn(true)
                       .with_payload_size(0)
                       .with_seqno(isn));
      test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(6));
      test.execute(ExpectNoSegment{});
      test.execute(WriteBytes{"abcdefg"});
      test.execute(ExpectSegment{}.with_no_flags().with_data("abcdef"));
      test.execute(ExpectNoSegment{});
    }

    {
      const size_t MIN_WIN = 5;
      const size_t MAX_WIN = 100;
      const size_t N_REPS = 1000;
      for (size_t i = 0; i < N_REPS; ++i) {
        size_t len = MIN_WIN + rd() % (MAX_WIN - MIN_WIN);
        TCPConfig cfg;
        WrappingInt32 isn(rd());
        cfg.fixed_isn = isn;

        TCPSenderTestHarness test{"Window " + to_string(i), cfg};
        test.execute(ExpectSegment{}
                         .with_no_flags()
                         .with_syn(true)
                         .with_payload_size(0)
                         .with_seqno(isn));
        test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(len));
        test.execute(ExpectNoSegment{});
        test.execute(WriteBytes{string(2 * N_REPS, 'a')});
        test.execute(ExpectSegment{}.with_no_flags().with_payload_size(len));
        test.execute(ExpectNoSegment{});
      }
    }

    {
      TCPConfig cfg;
      WrappingInt32 isn(rd());
      cfg.fixed_isn = isn;

      TCPSenderTestHarness test{"Window growth is exploited", cfg};
      test.execute(ExpectSegment{}
                       .with_no_flags()
                       .with_syn(true)
                       .with_payload_size(0)
                       .with_seqno(isn));
      test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(4));
      test.execute(ExpectNoSegment{});
      test.execute(WriteBytes{"0123456789"});
      test.execute(ExpectSegment{}.with_no_flags().with_data("0123"));
      test.execute(AckReceived{WrappingInt32{isn + 5}}.with_win(5));
      test.execute(ExpectSegment{}.with_no_flags().with_data("45678"));
      test.execute(ExpectNoSegment{});
    }

    {
      printf("--------------------------------------\n");
      TCPConfig cfg;
      WrappingInt32 isn(rd());
      cfg.fixed_isn = isn;

      TCPSenderTestHarness test{"FIN flag occupies space in window", cfg};
      test.execute(ExpectSegment{}
                       .with_no_flags()
                       .with_syn(true)
                       .with_payload_size(0)
                       .with_seqno(isn));
      test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(7));
      test.execute(ExpectNoSegment{});
      test.execute(WriteBytes{"1234567"});
      printf("11\n");
      test.execute(Close{});
      printf("22\n");
      test.execute(ExpectSegment{}.with_no_flags().with_data("1234567"));
      printf("33\n");
      // ?????????????????????????????????????????????
      test.execute(ExpectNoSegment{}); // window is full
      printf("44\n");
      test.execute(AckReceived{WrappingInt32{isn + 8}}.with_win(1));
      printf("55\n");
      test.execute(ExpectSegment{}.with_fin(true).with_data(""));
      printf("66\n");
      test.execute(ExpectNoSegment{});
    }

    {
      TCPConfig cfg;
      WrappingInt32 isn(rd());
      cfg.fixed_isn = isn;
      printf("----------------------------\n");
      printf("66\n");
      TCPSenderTestHarness test{"FIN flag occupies space in window (part II)",
                                cfg};
      printf("55\n");
      test.execute(ExpectSegment{}
                       .with_no_flags()
                       .with_syn(true)
                       .with_payload_size(0)
                       .with_seqno(isn));
      printf("44\n");
      test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(7));
      printf("33\n");
      test.execute(ExpectNoSegment{});
      printf("22\n");
      test.execute(WriteBytes{"1234567"});
      printf("11\n");
      test.execute(Close{});
      printf("00\n");
      test.execute(ExpectSegment{}.with_no_flags().with_data("1234567"));
      printf("11\n");
      test.execute(ExpectNoSegment{}); // window is full
      printf("22\n");
      test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(8));
      printf("33\n");
      // ??????????????????~
      test.execute(ExpectSegment{}.with_fin(true).with_data(""));
      printf("44\n");
      test.execute(ExpectNoSegment{});
      printf("------------------------\n");
    }

    {
      TCPConfig cfg;
      WrappingInt32 isn(rd());
      cfg.fixed_isn = isn;
      printf("11\n");
      TCPSenderTestHarness test{
          "Piggyback FIN in segment when space is available", cfg};
      printf("22\n");
      test.execute(ExpectSegment{}
                       .with_no_flags()
                       .with_syn(true)
                       .with_payload_size(0)
                       .with_seqno(isn));
      printf("33\n");
      test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(3));
      printf("44\n");
      test.execute(ExpectNoSegment{});
      printf("55\n");
      test.execute(WriteBytes{"1234567"});
      printf("66\n");
      test.execute(Close{});
      printf("77\n");
      test.execute(ExpectSegment{}.with_no_flags().with_data("123"));
      printf("88\n");
      test.execute(ExpectNoSegment{}); // window is full
      printf("99\n");
      test.execute(AckReceived{WrappingInt32{isn + 1}}.with_win(8));
      printf("100\n");
      test.execute(ExpectSegment{}.with_fin(true).with_data("4567"));
      printf("101\n");
      test.execute(ExpectNoSegment{});
    }
  } catch (const exception &e) {
    cerr << e.what() << endl;
    return 1;
  }

  return EXIT_SUCCESS;
}
