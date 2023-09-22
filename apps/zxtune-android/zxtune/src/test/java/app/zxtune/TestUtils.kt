package app.zxtune

import org.mockito.MockedConstruction
import org.mockito.Mockito
import org.mockito.kotlin.KStubbing
import org.mockito.kotlin.stub

object TestUtils {
    inline fun <reified T : Any> construct(crossinline stubbing: KStubbing<T>.(T) -> Unit): MockedConstruction<T> =
        Mockito.mockConstruction(T::class.java) { mock, _ ->
            mock.stub(stubbing)
        }

    inline val <reified T> MockedConstruction<T>.constructedInstance: T
        get() = constructed().run {
            first().also {
                require(size == 1)
            }
        }
}
