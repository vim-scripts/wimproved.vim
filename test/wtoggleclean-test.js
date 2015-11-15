var assert = require('assert')
var crypto = require('crypto')
var fs = require('fs')
var path = require('path')
var resemble = require('node-resemble-js')
var execSync = require('child_process').execSync

resemble.outputSettings({
    errorType: 'movement',
    transparency: 0.4
});

var productName = execSync('reg query "HKLM\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion" /v ProductName').toString();
var refType = productName.indexOf('Server') != -1 ? 'server' : 'desktop';

var pluginPath = path.resolve(__dirname, '../');
function configure(args) {
    var options = [
        '-N', // Turn off compatibility mode
        '-u', 'NORC', // Don't load .vimrc
        '-U', 'NORC', // Don't load .gvimrc
        // Disable swap files, set the runtime path to target this repository, disable the startup message
        '--cmd', '"set noswapfile | set rtp+=' + pluginPath + ' | set shortmess+=I"',
        // Disable cursor blink so there are no timing issues with screenshots
        '+"set guicursor=n:blinkon0"',
        '+"set title"',
        '+"set titlestring=wimproved.vim"'];
    return options.concat(args);
}

function launchVimAndTakeScreenshot(outputPath, options, complete) {
    var spawn = require('child_process').spawn;
    var process = spawn('gvim', options, { windowsVerbatimArguments: true });
    process.on('error', function(error) {
        console.error(error)
    });

    process.on('close', function(code) {
        complete();
    });

    try {
        var exec = require('child_process').execSync;
        exec('powershell -ep Bypass ./test/Take-Vim-Screenshot.ps1 ' + outputPath);
    } finally {
        process.kill('SIGTERM');
    }
}

function imageName(dir, id) {
    return path.join(dir, id + '-result.png');
}

function imageDiffName(dir, id) {
    return path.join(dir, id + '-diff.png');
}

function imageRefName(dir, id) {
    return path.join(dir, id + '-ref.png');
}

function takeScreenshotAndCompare(outputDir, id, referenceImage, vimSettings, done) {
    var config = configure(vimSettings);

    var filename = imageName(outputDir, id);
    function afterScreenshot(error) {
        if (error) {
            done(error)
        }

        var screenshot = fs.readFileSync(filename);
        var reference = fs.readFileSync(referenceImage);
        fs.writeFileSync(imageRefName(outputDir, id), reference);
        var api = resemble(screenshot).compareTo(reference).onComplete(function(data) {

            var stream = fs.createWriteStream(imageDiffName(outputDir, id));
            stream.on('finish', function() {
                if (Number(data.misMatchPercentage) > 0.01) {
                    done(new Error('Visual difference of ' + Number(data.misMatchPercentage) + '% detected.'));
                } else {
                    done();
                }
            });

            data.getDiffImage().pack().pipe(stream);
        });
    }

    launchVimAndTakeScreenshot(filename, config, afterScreenshot);
}

describe(':WToggleClean', function() {
    this.timeout(5000);

    var uniqueId = crypto.randomBytes(6).toString('hex');

    var outputDir = path.join('test-output', uniqueId);
    fs.existsSync('test-output') || fs.mkdirSync('test-output');
    fs.existsSync(outputDir) || fs.mkdirSync(outputDir);

    var tests = [
        {
            desc: 'should look clean with the default theme',
            ref: 'clean_default.png',
            args: ['+WToggleClean']
        },
        {
            desc: 'should look clean with a dark theme',
            ref: 'clean_dark.png',
            args: ['+"colorscheme desert"', '+WToggleClean']
        },
        {
            desc: 'should update window brush when color scheme changes',
            ref: 'clean_dark.png',
            args: ['+WToggleClean', '+"colorscheme desert"']
        },
        {
            desc: 'should look clean with guioptions already disabled',
            ref: 'clean_dark.png',
            args: ['+"set guioptions="', '+"set columns=80"', '+"colorscheme desert"', '+WToggleClean']
        },
        {
            desc: 'should restore default state with default color scheme',
            ref: 'default.png',
            args: ['+WToggleClean', '+redraw', '+WToggleClean']
        },
        {
            desc: 'should restore default state with dark color scheme',
            ref: 'default_dark.png',
            args: ['+"colorscheme desert"', '+WToggleClean', '+redraw', '+WToggleClean']
        }
    ]

    tests.forEach(function(test, i) {
        it('@' + i + ' ' + test.desc, function(done) {
            takeScreenshotAndCompare(outputDir, i, path.join('test', 'ref', refType, test.ref), test.args, done);
        });
    });
});

