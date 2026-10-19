// Optional Node validation of the shipped browser view; no frontend packages required.
const assert = require('node:assert/strict');
const fs = require('node:fs');
const vm = require('node:vm');

class Element {
  constructor() { this.children = []; this.style = {}; this.value = ''; this.ownText = ''; this.listeners = {}; }
  addEventListener(name, callback) { this.listeners[name] = callback; }
  set textContent(value) { this.ownText = String(value); this.children = []; }
  get textContent() { return this.ownText + this.children.map(c => c.textContent).join(' '); }
  append(...children) { this.children.push(...children); }
  replaceChildren(...children) { this.children = children; }
}
const elements = new Map();
const document = {
  createElement: () => new Element(),
  createTextNode: value => { const n = new Element(); n.textContent = value; return n; },
  getElementById: id => {
    if (!elements.has(id)) elements.set(id, new Element());
    return elements.get(id);
  },
};
const asset = fs.readFileSync(process.argv[2], 'utf8');
const script = asset.split('<script>')[1].split('</script>')[0];
// Compile the whole script, then evaluate its model and rendering without the polling loop.
new vm.Script(script);
const context = vm.createContext({ document, window: {} });
vm.runInContext(script.slice(0, script.indexOf("document.querySelectorAll('[data-view]')")), context);
const evaluate = code => vm.runInContext(code, context);

evaluate(`state.report = {
  fixture: 'Fixture', discovery_id: 'current', discovered_at_ms: 1000,
  scenarios: [
    { id: 0, accepted: true, name: 'Passing', test_name: 'Passing', uri: 'test.feature', line: 3 },
    { id: 1, accepted: true, name: 'Failing', test_name: 'Failing', uri: 'test.feature', line: 7 },
    { id: 2, accepted: false, name: 'Rejected', test_name: 'Rejected', uri: 'test.feature', line: 9 },
    { id: 3, accepted: true, name: 'Not run', test_name: 'Not run', uri: 'test.feature', line: 11 }
  ],
  rules: [{ name: '<rule>', description: 'Business requirements', kind: 'business',
    requirements: [{ tag: 'Invariant', description: '<b>Requirement</b>' }],
    events: [
      { kind: 'coverage', tag: 'Invariant', scenario_id: 0, scenario: 'Passing', message: 'Observed', uri: 'test.feature', line: 4 },
      { kind: 'coverage', tag: 'Invariant', scenario_id: 1, scenario: 'Failing', message: 'Observed', uri: 'test.feature', line: 8 }
    ]
  }]
};
window.zucchiniResult({fixture:'Fixture', test_name:'Passing', status:'passed', discovery_id:'current'});
window.zucchiniResult({fixture:'Fixture', test_name:'Failing', status:'failed', discovery_id:'current'});`);
assert.equal(evaluate('requirement(state.report.rules[0], {tag:"Invariant"}).label'), 'mixed');
assert.equal(evaluate('status(state.report.scenarios[3])'), 'not_run');
assert.equal(evaluate('status(state.report.scenarios[2])'), 'rejected');
assert.equal(evaluate('requirement(state.report.rules[0], {tag:"Absent"}).label'), 'uncovered');
evaluate('state.report.rules[0].events = [{kind:"coverage",tag:"OnlyRejected",scenario_id:2}]');
assert.equal(evaluate('requirement(state.report.rules[0], {tag:"OnlyRejected"}).label'), 'rejected_only');
evaluate(`window.zucchiniResult({fixture:'Fixture',test_name:'Passing',status:'passed',discovery_id:'previous'})`);
assert.equal(evaluate('status(state.report.scenarios[0])'), 'stale');
evaluate(`window.zucchiniResult({fixture:'Fixture',test_name:'Passing',status:'started',discovery_id:'current'})`);
assert.equal(evaluate('status(state.report.scenarios[0])'), 'incomplete');
evaluate('state.report.rules[0].events = []');
for (const view of ['requirements', 'scenarios', 'violations']) {
  context.view = view;
  evaluate('state.view = view; render(true)');
  assert.ok(elements.get('content').children.length);
}
evaluate(`state.view = 'requirements'; render(true)`);
assert.ok(elements.get('content').textContent.includes('<b>Requirement</b>'));

context.location = {uri: '/project/Features/nested/Order #%.feature', line:4, column:2,
  feature_relative_path:'nested/Order #%.feature'};
const renderedSource = evaluate('source(location)');
assert.equal(renderedSource.textContent, 'nested/Order #%.feature:4:2');
assert.ok(renderedSource.children.every(child => child.href === undefined));
context.location = {uri: String.raw`C:\Features\café.feature`, line:8};
assert.equal(evaluate('source(location).textContent'), 'café.feature:8');

evaluate(`state.report.rules[0].requirements = [
  {tag: 'Declared first', value: 20, description: 'First in source'},
  {tag: 'Declared second', value: 1, description: 'Second in source'}
]; state.view = 'requirements'; render(true)`);
const orderedText = elements.get('content').textContent;
assert.ok(orderedText.indexOf('Declared first') < orderedText.indexOf('Declared second'));

console.log('Dashboard status, rendering, and source-location checks passed');

evaluate(`state.report.sources = {'test.feature': [
  'Feature: Test', '  Background:', '    Given setup', '  Scenario: First',
  '    Then <value>', '  Scenario: Second', '    Then different'
]};
state.report.scenarios = [
 {id:0, uri:'test.feature', line:4, steps:[{line:3,text:'setup'},{line:5,text:'42'}]},
 {id:1, uri:'test.feature', line:6, steps:[{line:3,text:'setup'},{line:7,text:'different'}]}
];
state.report.rules[0].events = [
 {scenario_id:0,uri:'test.feature',line:5,kind:'coverage',tag:'Declared first',message:'Witness A'},
 {scenario_id:0,uri:'test.feature',line:5,kind:'violation',severity:'error',tag:'Declared first',message:'Violation B'},
 {scenario_id:1,uri:'test.feature',line:3,kind:'coverage',message:'Other scenario'}
];`);
const inline = evaluate('inlineScenario(state.report.scenarios[0])');
assert.ok(inline.textContent.includes('Given setup'));
assert.ok(inline.textContent.includes('→ 42'));
assert.ok(inline.textContent.includes('Witness A'));
assert.ok(inline.textContent.includes('Violation B'));
assert.ok(!inline.textContent.includes('Other scenario'));
assert.ok(!inline.textContent.includes('Scenario: Second'));
assert.ok(inline.children.some(row => row.className === 'source-line violation' && row.tabIndex === 0));

// Requirement previews are lazy, scoped to their rule, and distinguish the selected event.
evaluate(`state.report.rules.push({name:'Other rule', requirements:[], events:[
 {scenario_id:0,uri:'test.feature',line:5,kind:'coverage',message:'Unrelated rule'}
]});`);
const preview = evaluate('witnessDetails(state.report.rules[0], [state.report.rules[0].events[0]])');
assert.ok(!preview.textContent.includes('Given setup'));
preview.open = true;
preview.listeners.toggle();
assert.ok(preview.textContent.includes('Given setup'));
assert.ok(preview.textContent.includes('Witness A'));
assert.ok(preview.textContent.includes('Violation B'));
assert.ok(!preview.textContent.includes('Unrelated rule'));
const snippet = preview.children[1].children.at(-1);
assert.ok(snippet.children.some(row => row.className.includes('selected-observation')));
const childCount = preview.children[1].children.length;
preview.listeners.toggle();
assert.equal(preview.children[1].children.length, childCount);
assert.ok(evaluate('inlineScenario(state.report.scenarios[0])').textContent.includes('Unrelated rule'));

// DSL constraints stay quiet until violated and never enter coverage totals.
evaluate(`state.report.rules.push({name:'Structural protocol', description:'DSL metadata', kind:'dsl',
 requirements:[{tag:'Completed purchase',description:'Each purchase needs an outcome'},
              {tag:'Silent constraint',description:'No failure here'}], events:[]});
 state.view='requirements'; render(true);`);
assert.ok(!elements.get('content').textContent.includes('Structural protocol'));
const declaredBusinessCount = elements.get('metrics').children[0].textContent;
evaluate(`state.report.rules.at(-1).events.push({scenario_id:0,scenario:'First',uri:'test.feature',line:5,
 kind:'violation',tag:'Completed purchase',severity:'error',message:'Missing outcome'});
 render(true);`);
assert.equal(elements.get('metrics').children[0].textContent, declaredBusinessCount);
assert.ok(elements.get('content').textContent.includes('Completed purchase'));
assert.ok(!elements.get('content').textContent.includes('Silent constraint'));
// Count coverage declarations independently of structural declarations.
assert.equal(evaluate("state.report.rules.filter(r => r.kind !== 'dsl').flatMap(r => r.requirements).length"), 2);
evaluate(`state.view='violations'; render(true);`);
const violationsContent = elements.get('content');
assert.ok(violationsContent.textContent.includes('Given setup'));
const dslCard = violationsContent.children.find(c => c.textContent.includes('Structural protocol'));
assert.ok(dslCard.textContent.includes('Each purchase needs an outcome'));
assert.ok(dslCard.textContent.includes('Missing outcome'));
assert.ok(!dslCard.textContent.includes('Witness A'));
assert.ok(dslCard.children.some(c => c.children.some(row => (row.className || '').includes('selected-observation'))));

// Scenarios without observations get their own section; silent DSL checks are not evidence.
evaluate(`state.report.scenarios.push({id:2,name:'Unobserved',accepted:true,uri:'test.feature',line:6,steps:[]});
 state.view='scenarios'; render(true);`);
const scenarioSections = elements.get('content').children;
assert.equal(scenarioSections.length, 2);
assert.ok(scenarioSections[0].textContent.includes('Scenarios with rule observations (2)'));
assert.ok(!scenarioSections[0].textContent.includes('Unobserved'));
assert.ok(scenarioSections[1].textContent.includes('Scenarios without rule observations (1)'));
assert.ok(scenarioSections[1].textContent.includes('Unobserved'));

// Each section groups scenarios by feature, then Gherkin rule, in source order.
evaluate(`state.report.scenarios = [
  {id:0, name:'Late refund', feature:'Shop', gherkin_rule:'Refunds', accepted:true, uri:'shop.feature', line:9, steps:[]},
  {id:1, name:'Browse', feature:'Shop', gherkin_rule:'', accepted:true, uri:'shop.feature', line:3, steps:[]},
  {id:2, name:'Early refund', feature:'Shop', gherkin_rule:'Refunds', accepted:true, uri:'shop.feature', line:6, steps:[]},
  {id:3, name:'Count', feature:'Stock', gherkin_rule:'', accepted:true, uri:'C:\\\\features\\\\stock.feature', line:2, steps:[]}];
 state.report.rules = []; render(true);`);
const [grouped] = elements.get('content').children;
const groups = grouped.children.filter(child => child.className === 'feature-group');
assert.deepEqual(groups.map(group => group.children[0].textContent), ['Shop', 'Stock']);
const shopText = groups[0].textContent;
assert.ok(shopText.indexOf('Browse') < shopText.indexOf('Rule: Refunds'));
assert.ok(shopText.indexOf('Early refund') < shopText.indexOf('Late refund'));
const ruleGroups = groups[0].children.filter(child => child.className === 'rule-group');
assert.equal(ruleGroups.length, 1);
assert.ok(ruleGroups[0].textContent.includes('Early refund') && ruleGroups[0].textContent.includes('Late refund'));
assert.ok(!ruleGroups[0].textContent.includes('Browse'));

evaluate(`state.report.scenarios = ['Passing', 'Failing', 'Rejected', 'Not run'].map((name, id) =>
  ({id, name, test_name:name, accepted:id !== 2, uri:'test.feature', line:1, steps:[]}));
  state.report.rules = [];`);

// Model browser XML nodes so importer identity, replacement and status handling
// can be checked without a browser dependency.
context.DOMParser = class {
  parseFromString(xml, mime) {
    assert.equal(mime, 'application/xml');
    const cases = JSON.parse(xml).map(data => ({
      getAttribute: key => data[key] || null,
      querySelector: selector => {
        if (selector === 'failure, error') return data.failure ? {textContent: data.failure} : null;
        if (selector === 'skipped') return data.skipped ? {} : null;
        return null;
      },
    }));
    return {
      querySelector: selector => selector === 'testsuite' ? {} : null,
      querySelectorAll: () => cases,
    };
  }
};
evaluate(`importCTest(JSON.stringify([
  {name:'Fixture.Passing/Passing', time:'0.125'},
  {name:'Fixture.Failing', failure:'Expected failure'},
  {name:'AnotherFixture.Not run/Not run'},
  {name:'Fixture.Not run/Not run', skipped:true}
]))`);
assert.equal(evaluate('status(state.report.scenarios[0])'), 'passed');
assert.equal(evaluate('resultFor(state.report.scenarios[0]).duration_ms'), 125);
assert.equal(evaluate('status(state.report.scenarios[1])'), 'failed');
assert.equal(evaluate('status(state.report.scenarios[3])'), 'skipped');
evaluate(`window.zucchiniCTest(JSON.stringify([{name:'Fixture.Passing/Passing'}]))`);
assert.equal(evaluate('status(state.report.scenarios[1])'), 'not_run');
console.log('CTest result import and replacement checks passed');

// Newer CMake GoogleTest discovery uses a GetParam comment instead of /value.
evaluate(`importCTest(JSON.stringify([
  {name:'Fixture.Passing  # GetParam() = Passing', time:'0.125'},
  {name:'Fixture.Failing  # GetParam() = Failing', failure:'Expected failure'},
  {name:'Fixture.Not run  # GetParam() = Not run', skipped:true},
  {name:'Fixture.Rejected  # GetParam() = Wrong scenario'}
]))`);
assert.equal(evaluate('status(state.report.scenarios[0])'), 'passed');
assert.equal(evaluate('status(state.report.scenarios[1])'), 'failed');
assert.equal(evaluate('status(state.report.scenarios[3])'), 'skipped');
assert.equal(evaluate('state.results.size'), 3);
console.log('CMake GetParam test name import checks passed');

// Polling unchanged CTest data must preserve expanded observation DOM and previews.
evaluate(`state.view = 'requirements';
state.report.rules = [{name:'Rule', kind:'business', requirements:[{tag:'Requirement'}],
  events:[{scenario_id:0, kind:'coverage', tag:'Requirement', uri:'test.feature', line:1, message:'Witness'}]}];
window.zucchiniCTest(JSON.stringify([{name:'Fixture.Passing/Passing', time:'0.125'}]));`);
function findObservation(element) {
  if (element.listeners?.toggle) return element;
  for (const child of element.children || []) {
    const found = findObservation(child);
    if (found) return found;
  }
}
const expandedObservation = findObservation(elements.get('content'));
assert.ok(expandedObservation);
expandedObservation.open = true;
expandedObservation.listeners.toggle();
const expandedChildren = expandedObservation.children.length;
for (let refresh = 0; refresh < 3; refresh++) {
  evaluate(`window.zucchiniCTest(JSON.stringify([{name:'Fixture.Passing/Passing', time:'0.125'}])); render();`);
  assert.equal(findObservation(elements.get('content')), expandedObservation);
  assert.equal(expandedObservation.open, true);
  assert.equal(expandedObservation.children.length, expandedChildren);
}
// Actual execution changes still update the dashboard.
evaluate(`window.zucchiniCTest(JSON.stringify([{name:'Fixture.Passing/Passing', failure:'New failure'}]));`);
assert.equal(evaluate('status(state.report.scenarios[0])'), 'failed');
assert.notEqual(findObservation(elements.get('content')), expandedObservation);
console.log('Unchanged result refresh preserves expanded observations');

// Table rows highlight their cells; a table step is unjustified while any cell is.
evaluate(`state.report.sources = {'cells.feature': [
  'Feature: Cells', '  Scenario: Basket', '    Then my basket contains',
  '      | item  | note    | shelf |', '      | apple | crunchy | A3    |', '    And the checkout button is shown'
]};
state.report.scenarios = [{id:0, uri:'cells.feature', line:2, accepted:true, name:'Basket', test_name:'Basket',
  steps:[{line:3,text:'my basket contains',argument:{kind:'dataTable',rows:[{item:'apple',note:'crunchy',shelf:'A3'}],
    ignoredKeys:['shelf'],cellLocations:[{item:{line:5,column:9},note:{line:5,column:17},shelf:{line:5,column:27}}]}},
    {line:6,text:'the checkout button is shown'}],
  asserted_facts:3, unjustified_facts:[
    {step_index:0, line:5, column:17, cell:{row:0, key:'note'}},
    {step_index:1, line:6, column:5}]}];
state.report.rules = [{name:'Basket', kind:'business', requirements:[{tag:'Item', description:'Lines list items'}],
  events:[{scenario_id:0, uri:'cells.feature', line:3, kind:'coverage', tag:'Item', message:'Item named',
    cell:{row:0, key:'item', line:5, column:9}}]}];`);
const cells = evaluate('inlineScenario(state.report.scenarios[0])').children;
const rowAt = line => cells.find(row => row.children?.[0]?.textContent === String(line));
assert.equal(rowAt(3).className, 'source-line unjustified');
assert.ok(rowAt(3).textContent.includes("Cell 'note' in row 1"));
assert.equal(rowAt(5).className, 'source-line', 'Rows highlight cells, not the line');
const spans = rowAt(5).children[1].children.filter(child => child.className?.startsWith('cell'));
assert.deepEqual(spans.map(span => [span.className, span.textContent]),
    [['cell coverage', ' apple '], ['cell unjustified', ' crunchy '], ['cell ignored', ' A3    ']]);
assert.equal(spans[0].title, 'Basket / Item [Unspecified] · coverage: Item named');
assert.match(spans[1].title, /^Not tied to a requirement: 'note' in row 1/);
assert.match(spans[2].title, /^Ignored:/);
assert.equal(rowAt(6).className, 'source-line unjustified');
assert.ok(rowAt(6).textContent.includes('Outcome step'));
assert.ok(rowAt(6).textContent.includes('Not tied to a requirement'));
// Covered cells mark their step as covered once nothing is unjustified.
evaluate('state.report.scenarios[0].unjustified_facts = []');
assert.equal(evaluate('inlineScenario(state.report.scenarios[0])').children
    .find(row => row.children?.[0]?.textContent === '3').className, 'source-line coverage');
evaluate(`state.report.scenarios[0].unjustified_facts = [
    {step_index:0, line:5, column:17, cell:{row:0, key:'note'}}, {step_index:1, line:6, column:5}]`);
// Rule previews show only that rule's observations.
assert.ok(!evaluate('inlineScenario(state.report.scenarios[0], state.report.rules[0])')
    .textContent.includes('Not tied to a requirement'));
console.log('Cell observations and unjustified assertions render at their source lines');

// Hovers list observations strong to weak with a badge beside each requirement;
// unspecified sorts last among business requirements.
evaluate(`state.report.rules = [
  {name:'Loose', kind:'business', requirements:[{tag:'Note', description:'', strictness:'unspecified'}],
   events:[{scenario_id:0, uri:'cells.feature', line:6, kind:'coverage', tag:'Note', message:'Loose check'}]},
  {name:'Structure', kind:'dsl', requirements:[{tag:'Shape', description:''}],
   events:[{scenario_id:0, uri:'cells.feature', line:6, kind:'violation', severity:'warning', tag:'Shape', message:'Shape check'}]},
  {name:'Tight', kind:'business', requirements:[{tag:'Button', description:'', strictness:'exact'}],
   events:[{scenario_id:0, uri:'cells.feature', line:6, kind:'coverage', tag:'Button', message:'Tight check'}]}];`);
const hover = evaluate('inlineScenario(state.report.scenarios[0])').children
    .find(row => row.children?.[0]?.textContent === '6').children.at(-1);
const hoverText = hover.textContent;
assert.ok(hoverText.indexOf('Tight check') < hoverText.indexOf('Loose check'));
assert.ok(hoverText.indexOf('Loose check') < hoverText.indexOf('Shape check'));
const badges = [];
const collect = element => { if (element.className?.startsWith?.('strictness ')) badges.push(element); (element.children || []).forEach(collect); };
collect(hover);
assert.deepEqual(badges.map(b => [b.className, b.textContent]),
    [['strictness exact', 'Exact'], ['strictness unspecified', 'Unspecified']]);
evaluate(`state.view = 'requirements'; render(true)`);
assert.ok(elements.get('content').textContent.includes('Exact'));

// Statistics are advisory and live in their own tab.
evaluate(`state.report.scenarios.push({id:1, uri:'cells.feature', line:2, accepted:true, name:'Silent', test_name:'Silent',
  steps:[], asserted_facts:0, unjustified_facts:[]});
state.report.assertion_justification = {enforced:false, scenarios:2, scenarios_without_assertions:1,
  asserted_facts:3, unjustified_facts:2};
state.view = 'statistics'; render(true);`);
assert.ok(!elements.get('metrics').textContent.includes('Unjustified'));
const statistics = elements.get('content').textContent;
assert.ok(statistics.includes('These statistics are advisory only.'));
assert.ok(statistics.includes('no value, ratio or percentage among them should be used as a hard quality target'));
assert.ok(!statistics.includes('Advisory'));
assert.ok(statistics.includes('assertion statistics guide'));
assert.ok(statistics.includes('No requirement explains why these assertions were made'));
assert.ok(statistics.includes('tacit requirement'));
// Scenarios with untied assertions render again, highlighting only those assertions.
assert.ok(statistics.includes('And the checkout button is shown'));
assert.ok(statistics.includes("Cell 'note' in row 1"));
const untiedSources = [];
const findSources = element => { if (element.className === 'scenario-source') untiedSources.push(element); (element.children || []).forEach(findSources); };
findSources(elements.get('content'));
assert.equal(untiedSources.length, 1);
const untiedRows = untiedSources[0].children.filter(row => row.className?.startsWith('source-line'));
assert.deepEqual(untiedRows.filter(row => row.className !== 'source-line').map(row => [row.children[0].textContent, row.className]),
    [['3', 'source-line unjustified'], ['6', 'source-line unjustified']], 'Claimed assertions stay unhighlighted');
const untiedSpans = untiedRows.find(row => row.children[0].textContent === '5').children[1].children
    .filter(child => child.className?.startsWith('cell'));
assert.deepEqual(untiedSpans.map(span => span.className), ['cell unjustified']);
assert.ok(!untiedSources[0].textContent.includes('Tight check'));
assert.ok(statistics.includes('Scenarios without assertions'));
assert.ok(statistics.includes('Silent'));
assert.ok(!statistics.includes('njustified'));
const link = [];
const findLink = element => { if (element.href) link.push(element.href); (element.children || []).forEach(findLink); };
findLink(elements.get('content'));
assert.deepEqual(link, ['https://github.com/AnarchoSystems/Zucchini/blob/main/Doc/Rules.md#assertion-statistics']);
// Requirements rank strong to weak.
assert.ok(statistics.indexOf('Button') < statistics.indexOf('Note'));
evaluate('delete state.report.assertion_justification; render(true);');
assert.ok(elements.get('content').textContent.includes('records no asserted facts'));
console.log('Strictness badges and assertion statistics render');

