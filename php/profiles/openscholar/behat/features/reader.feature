Feature:
  Testing the reader tab.

  @api
  Scenario: Test the Reader tab
    Given I visit "john"
     When I click "Reader"
     Then I should see "Engadget rss"
