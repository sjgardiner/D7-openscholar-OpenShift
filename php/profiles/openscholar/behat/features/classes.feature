Feature:
  Testing the calsses tab.

  @api
  Scenario: Test the Classes tab
    Given I visit "john"
     When I click "Classes"
      And I click "John F. Kennedy"
     Then I should see the link "Wikipedia page on JFK"
      And I should see the link "Who was JFK?"
