Feature:
  Testing the search widget.

  @api
  Scenario: Search for a book witht the search widget.
     Given the widget "Search" is set in the "Publications" page
       And I visit "john/publications"
      When I fill in "search_block_form" with "The Little Prince"
      Then I should see the link "The Little Prince"
