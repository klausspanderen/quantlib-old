/* -*- mode: c++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 Copyright (C) 2013 Klaus Spanderen

 This file is part of QuantLib, a free-software/open-source library
 for financial quantitative analysts and developers - http://quantlib.org/

 QuantLib is free software: you can redistribute it and/or modify it
 under the terms of the QuantLib license.  You should have received a
 copy of the license along with this program; if not, please email
 <quantlib-dev@lists.sf.net>. The license is also available online at
 <http://quantlib.org/license.shtml>.

 This program is distributed in the hope that it will be useful, but WITHOUT
 ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 FOR A PARTICULAR PURPOSE.  See the license for more details.
*/

#include "observable.hpp"
#include "utilities.hpp"

#include <ql/patterns/observable.hpp>
#include <ql/math/randomnumbers/rngtraits.hpp>

#include <boost/thread.hpp>

using namespace QuantLib;
using namespace boost::unit_test_framework;


namespace {
    class A : public Observable {
    };

    class B : public Observer {
      public:
        B() { b = (double*) malloc(1024); }
        ~B() {free(b);
        }
        void update() { }
        
    private:
        double* b;
    };

    class Notifier {
      public:
        Notifier(const std::vector<boost::shared_ptr<Observable> >& observables)
        : urng_(12345ul),
          observables_(observables) { }

        void operator()() {
            for (Size i=0; i < 1000000; ++i) {
                const Size nextObservable
                    = Size(urng_.next().value * observables_.size());
                observables_[nextObservable]->notifyObservers();
            }
        }
      private:
        PseudoRandom::urng_type urng_;
        const std::vector<boost::shared_ptr<Observable> >& observables_;
    };

    class ObserverCreator {
      public:
        ObserverCreator(
            std::vector<boost::shared_ptr<Observer> >& observer,
            const std::vector<boost::shared_ptr<Observable> >& observables)
        : urng_(4567ul),
          observer_(observer),
          observables_(observables) {
        }

        void operator()() {
            for (Size i=0; i < 1000000; ++i) {
                const Size nextObserver
                    = Size(urng_.next().value * observer_.size());
                const boost::shared_ptr<Observer> anObserver(new B);
                observer_[nextObserver] = anObserver;

                for (Size j=0; j < 5; ++j) {
                    const Size nextObserver
                        = Size(urng_.next().value * observer_.size());
                    const Size nextObservable
                        = Size(urng_.next().value * observables_.size());

                    observer_[nextObserver]
                          ->registerWith(observables_[nextObservable]);
                }
            }
        }

      private:
        PseudoRandom::urng_type urng_;
        std::vector<boost::shared_ptr<Observer> >& observer_;
        const std::vector<boost::shared_ptr<Observable> >& observables_;
    };
}

void ObservableTest::testSimpleCall() {
    BOOST_TEST_MESSAGE("Testing Simple Call...");

    boost::shared_ptr<Observable> observable(new A);
    boost::shared_ptr<Observer> observer(new B);

    observer->registerWith(observable);
    observable->notifyObservers();
}

void ObservableTest::testMultiThreadingObservableStress() {
    BOOST_TEST_MESSAGE("Testing Multi Threading observable pattern...");

    const Size nObserver = 100;
    const Size nObservables = 500;

    std::vector<boost::shared_ptr<Observable> > observables(nObservables);
    for (Size i=0; i < nObservables; ++i) {
        observables[i] = boost::shared_ptr<Observable>(new A);
    }
    std::vector<boost::shared_ptr<Observer> > observer(nObserver);
    for (Size i=0; i < nObserver; ++i) {
        observer[i] = boost::shared_ptr<Observer>(new B);
    }

    Notifier notifier(observables);
    ObserverCreator observerCreator(observer, observables);

    boost::thread observerCreatorWorker(observerCreator);
    boost::thread notifierWorker1(notifier);
    boost::thread notifierWorker2(notifier);
    boost::thread notifierWorker3(notifier);
    boost::thread notifierWorker4(notifier);

    observerCreatorWorker.join();
    notifierWorker1.join();
    notifierWorker2.join();
    notifierWorker3.join();
    notifierWorker4.join();
}

void ObservableTest::testDirectInstantiation() {
    BOOST_TEST_MESSAGE("Testing Multi Threading observable pattern...");

    boost::shared_ptr<Observable> observable(new A);
    Observer Observer;

    Observer.registerWith(observable);
    observable->notifyObservers();
}

test_suite* ObservableTest::suite() {
    test_suite* suite = BOOST_TEST_SUITE("Observable tests");
//    suite->add(QUANTLIB_TEST_CASE(&ObservableTest::testSimpleCall));
//    suite->add(QUANTLIB_TEST_CASE(
//        &ObservableTest::testMultiThreadingObservableStress));
    suite->add(QUANTLIB_TEST_CASE(&ObservableTest::testDirectInstantiation));
    return suite;
}
