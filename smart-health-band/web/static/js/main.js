document.addEventListener('DOMContentLoaded', function() {
    const elements = document.querySelectorAll('main > *:not(script):not(style)');
    elements.forEach(el => {
        el.classList.add('animate-on-scroll');
    });

    function checkAnimations() {
        const animatedElements = document.querySelectorAll('.animate-on-scroll');
        animatedElements.forEach(el => {
            const elementTop = el.getBoundingClientRect().top;
            const elementVisible = 150;
            if (elementTop < window.innerHeight - elementVisible) {
                el.classList.add('visible');
            }
        });
    }

    window.addEventListener('scroll', checkAnimations);
    checkAnimations();
});