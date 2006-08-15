#ifndef __pbd_id_h__
#define __pbd_id_h__

#include <stdint.h>
#include <string>

#include <glibmm/thread.h>

namespace PBD {

class ID {
  public:
	ID ();
	ID (std::string);
	
	bool operator== (const ID& other) const {
		return _id == other._id; 
	}

	bool operator!= (const ID& other) const {
		return _id != other._id;
	}

	ID& operator= (std::string); 

	bool operator< (const ID& other) const {
		return _id < other._id;
	}

	void print (char* buf) const;
        std::string to_s() const;
	
	static uint64_t counter() { return _counter; }
	static void init_counter (uint64_t val) { _counter = val; }
	static void init ();

  private:
	uint64_t _id;
	int string_assign (std::string);

	static Glib::Mutex* counter_lock;
	static uint64_t _counter;
};

}
std::ostream& operator<< (std::ostream& ostr, const PBD::ID&);

#endif /* __pbd_id_h__ */
