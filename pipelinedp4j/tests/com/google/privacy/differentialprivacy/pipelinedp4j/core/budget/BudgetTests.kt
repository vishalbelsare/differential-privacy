/*
 * Copyright 2024 Google LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.google.privacy.differentialprivacy.pipelinedp4j.core.budget

import org.junit.runner.RunWith
import org.junit.runners.Suite

/** Provides a list of JUnit test classes to Bazel. When creating a new test class, add it here. */
@RunWith(Suite::class)
@Suite.SuiteClasses(
  AbsoluteBudgetPerOpSpecTest::class,
  NaiveBudgetAccountantTest::class,
  RelativeBudgetPerOpSpecTest::class,
  TotalBudgetTest::class,
)
class BudgetTests {}
