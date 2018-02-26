/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * Singleton.h
 *
 *  Created on: 30.08.2016
 *      Author: pdschbrt
 */

#ifndef UTILS_SINGLETON_H_
#define UTILS_SINGLETON_H_

template <typename T> class Singleton {
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
