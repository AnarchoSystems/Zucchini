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
