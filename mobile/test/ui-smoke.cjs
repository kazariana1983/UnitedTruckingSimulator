// Run with NODE_PATH pointing to an installed playwright package.
const {chromium}=require('playwright');
const assert=require('node:assert/strict');
(async()=>{
 const browser=await chromium.launch({headless:true,args:['--no-sandbox']});
 const context=await browser.newContext({viewport:{width:1000,height:700},hasTouch:true});
 const page=await context.newPage();const errors=[];page.on('pageerror',e=>errors.push(e.message));
 await page.goto('http://localhost:4173');await page.locator('#start').click();
 const box=await page.locator('#throttle').boundingBox();await page.mouse.move(box.x+20,box.y+20);await page.mouse.down();await page.waitForTimeout(400);await page.mouse.up();
 await page.locator('#pause').click();assert.equal(await page.locator('#state').textContent(),'Paused');
 await page.locator('#pause').click();await page.locator('#end').click();await page.locator('#close').click();
 await page.reload();await page.locator('#history').click();assert.match(await page.locator('#modal-body').textContent(),/ended/);await page.locator('#close').click();
 await page.locator('#start').click();await page.evaluate(()=>window.dispatchEvent(new Event('blur')));assert.equal(await page.locator('#state').textContent(),'Paused');
 // A reload recovers the checkpoint as interrupted, without claiming completion.
 await page.reload();await page.locator('#history').click();assert.match(await page.locator('#modal-body').textContent(),/interrupted/);await page.locator('#close').click();
 await page.setViewportSize({width:844,height:390});await page.screenshot({path:'/tmp/backing-lab-landscape.png'});
 assert.ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
 await page.setViewportSize({width:390,height:844});assert.ok(await page.evaluate(()=>document.documentElement.scrollWidth<=innerWidth));
 assert.deepEqual(errors,[]);console.log('UI smoke passed: controls, pause, history/reload, interruption recovery, phone layouts, no page errors');
 await browser.close();
})().catch(e=>{console.error(e);process.exit(1);});
