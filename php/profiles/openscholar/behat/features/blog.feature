Feature:
  Testing the blog tab.

  @api
  Scenario: Test the Blog tab
    Given I visit "john"
     When I click "Blog"
     Then I should see "First blog"
