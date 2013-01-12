Feature: Testing OpenScholar calendar page.

  @api
  Scenario: Test the Calendar tab
    Given I visit "john"
     When I click "Calendar"
     Then I should see "John F. Kennedy birthday"

  @javascript @wip
  Scenario: Test the popup for the events in the calendar.
    Given I visit "/john"
      And I click "Calendar"
      And I click "John F. Kennedy birthday"
     Then I should wait and see "Thursday, May 30, 2013 (All day)"
