/*
 * Singleton.h
 *
 *  Created on: 30.08.2016
 *      Author: pdschbrt
 */

#ifndef UTILS_SINGLETON_H_
#define UTILS_SINGLETON_H_

template <class T> class Singleton {
public:
  Singleton(const Singleton &s) = delete;
  Singleton(Singleton &&s) = delete;
  Singleton &operator=(const Singleton &s) = delete;
  Singleton &operator=(Singleton &&s) = delete;
  static T &Instance() {
    static T value;
    return value;
  }

protected:
  Singleton() = default;
  ~Singleton() = default;
};

#endif /* UTILS_SINGLETON_HH_ */
